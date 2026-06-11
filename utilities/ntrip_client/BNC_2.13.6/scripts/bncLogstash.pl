#!/usr/bin/env perl

# ========================================================================
# bncLogstash.pl
# ========================================================================
#
# Purpose: Reads BNC's logfile and converts the parsed metrics to InfluxDB
#          line protocol.
#
# Author  : Erwin Wiesensarter, April-2016
# Revision: $Header: https://software.rtcm-ntrip.org/svn/tags/BNC_2.13.6/scripts/bncLogstash.pl 9627 2022-02-21 09:07:55Z wiese $
# ========================================================================

# Uses
use strict;
use warnings;

BEGIN {
    use FindBin qw($Bin);
    use lib "$Bin";
}

use FindBin qw($Bin);
use Getopt::Long;
use File::Basename;
use File::Spec::Functions;
use List::MoreUtils qw(any uniq);
use Log::Log4perl qw(:easy);
use Time::Piece 1.30;
use Data::Dumper;
use POSIX qw(uname);

# BKG Perl libs
use Bnc;
use Common;

# Logging
Log::Log4perl->easy_init(
    {
      layout => '%d [%c] %p: %m%n',    # '[%F{1}-%L-%M]: %m%n'
      level  => $DEBUG                 # $TRACE $INFO
    }
);

# Options
my $bncConf   = {};
my ($home)    = glob "~";
my $corrMount = "n/a";
my $now       = Time::Piece->new;
my ($prog)    = fileparse($0);
my $hostname  = (uname)[1];
my $influx_db = 'realtime';

# Args
my $help          = 0;
my $fromBeginning = 0;
my $bncConfFile   = "";
my $intv          = 5;          # each epoch (default)
my $pppScene      = 0;
my $output        = "influx";
my @tags          = ();

GetOptions(
            'help'       => \$help,
            'from-begin' => \$fromBeginning,
            'conf=s'     => \$bncConfFile,
            'intv=s'     => \$intv,
            'scene=s'    => \$pppScene,
            'output=s'   => \$output,
            'tags=s'     => \@tags,
);

HELP_MESSAGE() if $help;

if ( $output && !( $output =~ /influx/i ) ) { LOGDIE "Undefined output \"$output\"\n", HELP_MESSAGE() }
$output = lc $output;
$bncConfFile || LOGDIE( "Please set option \"conf\".\n", HELP_MESSAGE() );
if ( $bncConfFile && !-s $bncConfFile ) {
    my $bncConfFile1 = catfile( $home, $bncConfFile );
    if   ( -s $bncConfFile1 ) { $bncConfFile = $bncConfFile1 }
    else                      { LOGDIE "BNC config file \"$bncConfFile\" does not exist\n" }
}
my %tags = ();
foreach (@tags) {
    s/^\s*//;
    s/\s+$//;
    my ( $k, $v ) = split ( /\s*=\s*/, $_, 2 );
    $tags{$k} = $v;
}

INFO ">>>>> Start $prog $bncConfFile ..";

# Should only run once
# TODO checks args for that
Common::isAlreadyRunning() && LOGDIE "Job is already running! Exit\n";

# -----------------------------------------------------------------------------
# Parse BNC config file
# -----------------------------------------------------------------------------
$bncConf   = Bnc::parseConfig($bncConfFile);     #  cmbMethod dataSource ?
$corrMount = $bncConf->{'PPP'}->{'corrMount'};
my $bncLogFile = $bncConf->{'General'}->{'logFile'};
my $date       = getLogFileDate($now);
$bncLogFile .= "_$date";                         # -> *_160425

-s "$bncLogFile" || LOGDIE("logfile \"$bncLogFile\" does not exist. Exit\n");
my ( $logFilName, $logFilDir ) = fileparse($bncLogFile);

my $mpts_def = $bncConf->{'General'}->{'mountPoints_parsed'};
my %mpLookup = map { $_->{'mp'}, $_->{'caster'} } @$mpts_def;

# -----------------------------------------------------------------------------
# Select and parse BNC logfile
# -----------------------------------------------------------------------------
my $data   = Bnc::parseLogfile( $bncLogFile, $intv, !$fromBeginning );
my @EPOCHS = @{ $data->{'EPOCHS'} };
my @sites  = map { $_->{'site'} } @EPOCHS;
@sites = uniq @sites;
my $nof_epochs = scalar @EPOCHS;

my @latencies = @{ $data->{'LATENCIES'} };
my @restarts  = @{ $data->{'RESTARTS'} };

# Some output
DEBUG(   "\n>   output   : $output\n>   BNC conf : $bncConfFile"
       . "\n>   BNC lfile: $bncLogFile\n>   sampling : $intv\n>   PPP-Scene: $pppScene"
       . "\n>   Sites    : @sites\n>   #epochs  : $nof_epochs\n>   tags     : @tags"
       . "\n>   Latencies: "
       . scalar @latencies
       . "\n>   Restarts : "
       . scalar @restarts );

if ( $nof_epochs < 1 && scalar @latencies < 1 ) {
    INFO "No epochs or latencies found. Do nothing";
    exit 0;
}

# -----------------------------------------------------------------------------
# Reformat data
# -----------------------------------------------------------------------------
my @data_influxFormated = ();

# Epochs
my $rcd = {};
foreach (@EPOCHS) {
    next if ( $_->{'dN'} == 0 || $_->{'dU'} == 0 );

    if ( $_->{'dN'} eq '-nan' || $_->{'dU'} eq '-nan' ) {
        next;
    }

    # Horizontal displacement (computation not available yet in influxDB)
    # influxDB kann sowas "SELECT sum(value * value) FROM cpu" leider NOCH nicht!
    # NEU in version 1.6: math functions SQRT,POW, usw. !!
    my $d2d = $_->{'dN'}**2 + $_->{'dE'}**2;    # Quadrat, um spaeter leichter den RMS zu berechnen
    my $d3d = $d2d + $_->{'dU'}**2;

    $rcd = {
        measurement => 'ppp',
        tags        => {
            mp        => $_->{'site'},
            corrMount => $corrMount || "NO",                       # SPP, empty geht derzeit nicht -> InfluxError
            caster    => $mpLookup{ $_->{'site'} } || 'Unknown',
            host      => $hostname,
            scene     => $pppScene,
            clientSW  => 'BNC'

              #pppvers   => $pppvers,
              #bncvers   => '2.12.0',
        },
        fields => {
                    dN  => $_->{'dN'} + 0.0,
                    dE  => $_->{'dE'} + 0.0,
                    dU  => $_->{'dU'} + 0.0,
                    d2d => sprintf ( "%.5f", $d2d ),
                    d3d => sprintf ( "%.5f", $d3d ),
                    TRP => $_->{'TRP'} + 0.0,
        },
        time => $_->{'time'}
    };
    push ( @data_influxFormated, $rcd );
}

# Latencies
my $lat_rcd = {};
foreach (@latencies) {
    $lat_rcd = {
                 measurement => 'stream_latency',
                 tags        => {
                           mp     => $_->{'mp'},
                           type   => $_->{'type'},
                           caster => $mpLookup{ $_->{'mp'} } || 'Unknown',
                           host   => $hostname,
                           scene  => $pppScene
                 },
                 fields => {
                             meanLat => $_->{'meanLat'},
                             epochs  => $_->{'epochs'},
                             gaps    => $_->{'gaps'} || 0
                 },
                 time => $_->{'time'}
    };
    push ( @data_influxFormated, $lat_rcd );
}

# BNC Starts
if ($pppScene) {
    my $restart_rcd = {};
    foreach (@restarts) {
        $restart_rcd = {
                         measurement => 'ppp',
                         tags        => {
                                   host  => $hostname,
                                   scene => $pppScene,
                                   event => 'BNC_Start'
                         },
                         fields => { text => "\"BNC " . $_->{'bncvers'} . " started\"" },
                         time   => $_->{'time'}
        };
        push ( @data_influxFormated, $restart_rcd );
    }
}

# -----------------------------------------------------------------------------
# Output
# -----------------------------------------------------------------------------
if ( scalar @data_influxFormated < 1 ) {
    INFO "No metrics found";
    exit 0;
}

# add tags from arguments
foreach my $rc (@data_influxFormated) {
    foreach my $k ( keys %tags ) {
        $rc->{'tags'}->{$k} = $tags{$k};
    }
}

if ( $output =~ /influx/ ) {
    my ( $i, $err ) = ( 0, 0 );
    foreach (@data_influxFormated) {
        my $l = Common::toLineProtocol($_);
        if ($l) {
            print STDOUT "$l\n";
            $i++;
        }
        else { $err++ }
    }
    DEBUG "Lines written: $i";
    DEBUG "Errorneous lines: $err";
}
else {
    LOGDIE("output not supported: $output");
}

##########################################################################
# Select date of the logfile we want to read
sub getLogFileDate {
    my ($tp) = @_;
    if ( $tp->hour == 0 && $tp->min < 5 ) {    # read last day
        $tp -= 300;
    }
    return $tp->strftime('%y%m%d');
}

sub HELP_MESSAGE {
    print <<EOI_HILFE;
$prog - reads BNC's logfile and writes metrics in formats that can be easily imported
by other tools, e.g. InfluxDB.

USAGE:
  $prog [paramter]

OPTIONS:
  -c|--conf        BNC config file for this job. Settings like PPP logpath
                   will be parsed from that config file.
                   Options -l and -b are not allowed together.
  -f|--from-begin  If set, the the logfile will be read from the beginning. Default is that the logfile is tailed.
  -o|--output      output format. So far only the InfluxDB line protocol (default).
  -i|--intv        sampling interval in seconds that shall be used in plots;
                   default: each epoch; for a daily logfile <30> seconds
                   should be usefull.
  -t|--tags <tags> comma separeted list of tags in format key=value,key2=value.
  -s|--scene       PPP scene number or identifier.
  -h|--help        show help contents.

EXAMPLES:
   $prog --conf path/to/BNC.bnc -i 5 -s 24 -f 2>bnc.err
   $prog --conf path/to/BNC.bnc -o influx -t proj=cost,name=Tim 2>bnc.err

Copyright (c) 2016 BKG Frankfurt <erwin.wiesensarter\@bkg.bund.de>
EOI_HILFE
    exit;
}
