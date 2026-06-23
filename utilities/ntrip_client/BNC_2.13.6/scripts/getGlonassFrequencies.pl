#!/usr/bin/env perl

#
# getGlonassFrequencies.pl - parse GLONASS slot & freq information from a logfile written by BNC in scanRTCM mode
#
# Revision: $Header: https://software.rtcm-ntrip.org/svn/tags/BNC_2.13.6/scripts/getGlonassFrequencies.pl 9668 2022-03-23 12:51:45Z wiese $
#

use strict;
use warnings;

my $scanRTCMlogfile = shift @ARGV;
defined ($scanRTCMlogfile) || die ( HELP_MESSAGE(), "\n" );

my %frq_of = parseGloFrequencies($scanRTCMlogfile);
die ("no GLO frequencies found in $scanRTCMlogfile\n") if ( scalar ( keys %frq_of ) < 1 );

foreach ( sort { $a <=> $b } keys %frq_of ) {
    print STDOUT $_ . ' ' . $frq_of{$_} . "\n";
}

# 22-03-23 08:46:19 USCL00CHL0: GLONASS Slot:Freq R07:5 R08:6 R12:-1 R13:-2 R14:-7 R17:4 R23:3 R24:2
sub parseGloFrequencies {
    my ($logfile) = @_;

    my %frq_of = ();
    open ( my $inp, '<', $logfile ) || die "could not open file '$logfile': $!\n";
    while ( my $ln = <$inp> ) {
        chomp $ln;
        next unless ( $ln =~ /GLONASS Slot:Freq/ );
        my $idx = index ( $ln, "Freq", 30 );
        next unless ($idx);
        my $slotsStr = substr ( $ln, $idx + 5 );
        $slotsStr =~ s/^[\s]*//g;
        my @hlp = split ( /\s+/, $slotsStr );
        foreach (@hlp) {
            my ( $sat, $frq ) = split ( ":", $_, 2 );
            my $satNr = int ( substr ( $sat, 1 ) );
            if ( exists $frq_of{$satNr} && $frq_of{$satNr} != $frq ) {
                warn ("frq differs for existing $sat:$frq_of{$satNr} in line: $ln\n");
            }
            $frq_of{$satNr} = int ($frq);
        }
    }
    close ($inp);
    return %frq_of;
}

sub HELP_MESSAGE {
    print <<EOI_KURZHILFE;
getGlonassFrequencies.pl - Parse GLONASS slot & freq information from a logfile written by BNC in scanRTCM mode

Usage: getGlonassFrequencies.pl /path/to/BNC-logfile

Arguments:
    -h|--help      print help
    -BNC-logfile   BNC-logfile with activated scanRTCM mode

Copyright (c) 2022 BKG Frankfurt <Erwin.Wiesensarter\@bkg.bund.de>
EOI_KURZHILFE
}

