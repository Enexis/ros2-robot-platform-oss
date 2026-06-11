#!/usr/bin/env perl

#
# sot.pl - parse BNC logfile(s) and create *.sot files that contain the observation types for each GNSS in RINEX header format
#
# Revision: $Header: https://software.rtcm-ntrip.org/svn/tags/BNC_2.13.6/scripts/sot.pl 10687 2025-07-02 16:45:11Z wiese $
#

use strict;
use warnings;
use File::Basename;
use File::Path;
use Getopt::Std;

my $prog = basename($0);

# Arguments
getopts( "hn:", \my %opts );
$opts{h} && HELP_MESSAGE();
my $caster = $opts{n} ? $opts{n} : "";
if ( scalar @ARGV < 1 ) {
    warn "usage error: no BNC logfile given\n";
    HELP_MESSAGE();
}
my @logFiles = @ARGV;

my %obsTyps_of;

# Parse each logfile
foreach my $logfile (@logFiles) {
    my $inp;
    if ( !open ( $inp, '<', $logfile ) ) {
        warn "could not open file '$logfile': $!";
        next;
    }
    while (<$inp>) {

        # # 16-03-21 10:25:38 MARS0: Observation Types: R     6   C1C L1C S1C C2P L2P S2P
        if ( $_ =~ /Observation Types:/ ) {
            chomp;
            my @fields = split ( /\s+/, $_ );
            my $mp     = $fields[2];
            $mp =~ s/:$//;
            my $satSys  = $fields[5];
            my @obsTyps = @fields[ 7 .. $#fields ];
            $obsTyps_of{$mp}->{$satSys} = \@obsTyps;

            # Check obs types
            if ( $satSys eq "G" ) {    # GPS see #65
                foreach my $otp (@obsTyps) {
                    if ( $otp eq "L2P" || $otp eq "C2P" ) {
                        warn ("$_: Observation Types 'L2P' and/or 'C2P' are not reasonable for GPS\n");
                    }
                }
            }
        }
    }
    close ($inp);
}

# Write .sot files
foreach my $mp ( keys %obsTyps_of ) {
    my $obstypes = obsTypes2str( $obsTyps_of{$mp} );
    next unless ($obstypes);

    my $fname = "${mp}.sot";
    writeSOT( $fname, $obstypes, $caster );
}

sub obsTypes2str {
    my ($obsTypes) = @_;

    my $obstypes_str = "";
    foreach my $sys (qw(G R E J C I S)) {
        next unless ( exists $obsTypes->{$sys} );
        my @obsTypes     = @{ $obsTypes->{$sys} };
        my $nof_obsTypes = scalar @obsTypes;
        my @currentTypes = splice ( @obsTypes, 0, 13 );
        my $types_str    = join ( ' ', @currentTypes );
        $obstypes_str .=
          sprintf ( "%-1s  %3s %-53.53s%-19s\n", $sys, $nof_obsTypes, $types_str, 'SYS / # / OBS TYPES' );
        while (@obsTypes) {    # we need a new line
            $types_str = join ( ' ', @obsTypes );
            $obstypes_str .= sprintf ( "       %-52.52s %-20s\n", $types_str, 'SYS / # / OBS TYPES' );
            @currentTypes = splice ( @obsTypes, 0, 13 );
        }
    }
    return $obstypes_str;
}

# Write the sotfile. If option -n is set, write the file into a directory with the name of the caster.
sub writeSOT {
    my ( $filename, $obsTypes, $caster ) = @_;

    if ($caster) {
        if ( !-d $caster ) {
            eval { mkpath($caster) };
            if ($@) {
                warn ("could not create path [$caster]: $@");
                return;
            }
        }
        $filename = $caster . "/" . $filename;
    }

    my $fh;
    if ( !open ( $fh, '>', $filename ) ) {
        warn "could not open file '$filename' for writing: $!";
        return;
    }
    print $fh $obsTypes;
    close ($fh);
}

sub HELP_MESSAGE {
    print <<EOI_HILFE;
$prog - parse observation types from BNC's logfile(s) and write them as *.sot files

USAGE:
  $prog [OPTIONS] <bnc-logfiles>

OPTIONS:
  -n           name or address of the scanned caster (optional)
  -h, --help   show this help

EXAMPLES:
   $prog -n euref-ip.net /home/user/log/bnc.log*

EOI_HILFE
    exit;
}

