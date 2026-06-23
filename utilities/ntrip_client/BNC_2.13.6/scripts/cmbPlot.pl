#!/usr/bin/env perl

# ========================================================================
# cmbPlot.pl
# ========================================================================
#
# Purpose: Script using gnuplot to plot BNC's combination results
#
# Comment: stored values:
#    in case of "single epoch" and "filter" combination
#    - residuals per satellite and AC
#    - resultant combined clock correction in [seconds]
#    - full clock in [m]
#    additional for "filter" combination
#    - estimated clock offset common for all satellites of a system and one AC
#    - estimated clock offset per satellite and AC
#
# Author: Andrea Stuerze
# ========================================================================

# Uses
use strict;
use warnings;
use File::Spec;
use Getopt::Long;
use Chart::Gnuplot;
use PDL;
use Data::Dumper qw(Dumper);
use diagnostics;
use File::Basename;
use Date::Manip;


my ($home) = glob "~";
# -----------------------------------------------------------------------------
# required to create a pdf
# -----------------------------------------------------------------------------
use PDF::API2;
use constant {
               mm   => 25.4 / 72,
               inch => 1 / 72,
               pt   => 1,
};    # There are 72 postscript points in an inch and there are 25.4 millimeters in an inch.

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
# init options
my $help     = 0;
my $plotType = "";
my @logFiles = ();

# read options from command line
GetOptions(
            'help'       => \$help,
            'plotType=s' => \$plotType,
            'logFiles=s' => \@logFiles,
);
HELP_MESSAGE() if $help;
@logFiles = split ( /[, ]+/, join ( ',', @logFiles ) );
unless (@logFiles) {
    print "ERROR: logfiles missing\n";
    exit;
}
print "    plotTpe: $plotType\n    logfiles: @logFiles\n\n";

# -----------------------------------------------------------------------------
# generate data sets for gnuplot
# -----------------------------------------------------------------------------
#my $old_epochSec = 0;
my $epochSec  = 0;
my $epochDiff = 0;

# for pdf generation
my ( $png, $page, $headline, $headline_text );
my $y0 = 180 / mm;
my ( $x, $y, $width, $height ) = ( 40 / mm, $y0, 130 / mm, 80 / mm );
my $dy = $height + 10 / mm;

# -----------------------------------------------------------------------------
# create pdf for plot results
# -----------------------------------------------------------------------------
my ( $inputFilename, $inputDir ) = fileparse( $logFiles[0] ); # , '\..*'
my $pdf_name = sprintf ( "%s.pdf", $inputFilename );
my $pdf = PDF::API2->new( -file => "$inputDir$pdf_name" );
my $font1 = $pdf->corefont('Helvetica-Bold');

# -----------------------------------------------------------------------------
# read logfile(s) and save data
# -----------------------------------------------------------------------------
#chdir ( "$home/tmp" ) || die "Could not change to dir $home/tmp\n";

my ( %CLK_CORR, %CLK_FULL, %RES, %OFFSET_AC, %OFFSET_SAT );    # %CLK
foreach (@logFiles) {
    #my $file = File::Spec->catfile( $inputDir, $_ );
    my $file =  $_;
    print "Parse logfile $file ...\n";
    my ( @array, $sys, $sat, $epo, $val, $ac );
    open ( INPUT, "<", $file ) || die "Could not open file '$file': $!";
    while (<INPUT>) {
        if ( $_ =~ /Clk Corr / ) {    # 2015-04-06 23:59:30.000 Clk Corr G06          0.2371     +-   0.0291
            @array = split ( /\s+/, $_ );
            $epo   = $array[0] . "T" . $array[1];
            $sat   = $array[4];
            $sys   = substr ( $sat, 0, 1 );
            $val   = $array[5];
            if ($val != 0.0) {
              push ( @{ $CLK_CORR{$sys}{$sat}{EPOCH} }, $epo );
              push ( @{ $CLK_CORR{$sys}{$sat}{DATA} },  $val );
            }
        }
        elsif ( $_ =~ /Full Clock / ) {    # 2015-04-06 23:59:30.000 Full Clock G04    81       -3205.8546
            @array = split ( /\s+/, $_ );
            $epo   = $array[0] . "T" . $array[1];
            $sat   = $array[4];
            $sys   = substr ( $sat, 0, 1 );
            $val   = $array[6];
            push ( @{ $CLK_FULL{$sys}{$sat}{EPOCH} }, $epo );
            push ( @{ $CLK_FULL{$sys}{$sat}{DATA} },  $val );
        }
        elsif ( $_ =~ /res = / ) {         # 2015-04-05 23:59:40.000 NRCAN G01    res =    -0.0003
            @array = split ( /\s+/, $_ );
            $epo   = $array[0] . "T" . $array[1];
            $ac    = $array[2];
            $sat   = $array[3];
            $sys   = substr ( $sat, 0, 1 );
            $val   = $array[6];
            push ( @{ $RES{$ac}{$sys}{$sat}{EPOCH} }, $epo );
            push ( @{ $RES{$ac}{$sys}{$sat}{DATA} },  $val );
        }
        elsif ( $_ =~ /AC  Offset / ) {  #                         AC  Offset DLR C          1.9002     +-  35.3112
            @array = split ( /\s+/, $_ );
            $epo   = $array[0] . "T" . $array[1];
            $sys   = $array[5];
            $ac    = $array[4];
            $val   = $array[6];
            if ($val != 0.0) {
              push ( @{ $OFFSET_AC{$sys}{$ac}{EPOCH} }, $epo );
              push ( @{ $OFFSET_AC{$sys}{$ac}{DATA} },  $val );
            }
        }
        elsif ( $_ =~ /Sat Offset / ) {    # 2015-04-05 23:59:40.000 Sat Offset BKG G01          0.5483     +- 100.0000
            @array = split ( /\s+/, $_ );
            $epo   = $array[0] . "T" . $array[1];
            $ac    = $array[4];
            $sat   = $array[5];
            $sys   = substr ( $sat, 0, 1 );
            $val   = $array[6];
            if ($val != 0.0) {
              push ( @{ $OFFSET_SAT{$ac}{$sys}{$sat}{EPOCH} }, $epo );
              push ( @{ $OFFSET_SAT{$ac}{$sys}{$sat}{DATA} },  $val );
            }
        }
    }
    close INPUT;
}    # -----  end foreach logfile -----

# -----------------------------------------------------------------------------
# plot several data sets
# -----------------------------------------------------------------------------
######### CLOCK CORR #####################
print "Plot CLOCK CORR ...\n";
$page = $pdf->page();
$page->mediabox('A4');
$headline = sprintf ( "Clock Corrections (%s)", $inputFilename );
$headline_text = $page->text;
$headline_text->font( $font1, 11 / pt );
$headline_text->translate( 15 / mm, 280 / mm );
$headline_text->text($headline);
$y = $y0 + $dy;
my $yrange = 20.0;
if ( $plotType eq "KF" ) {
    $yrange = 10.0;
}

# SYSTEM
foreach my $key_sys ( sort keys %CLK_CORR ) {
    print "    Plot CLOCK CORR for $key_sys\n";
    my @datasets;    # init datasets
    my $pngName = sprintf ( "%s_CLK_CORR_%s.png", $inputFilename, $key_sys );
    my $chart = Chart::Gnuplot->new(
                                     output   => $pngName,
                                     terminal => 'png',
                                     title    => "$key_sys", # G,R
                                     ylabel   => "Clock corrections [s]",
                                     yrange   => [ -$yrange, $yrange ],
                                     xlabel   => "Time [h]",
                                     timeaxis => 'x',
                                     xtics    => { labelfmt => '%H:%M', rotate => '-270' },
                                     legend => { position => "outside below", },
                                     grid   => 'on',
    );

    # SATELLITE
    foreach my $key_sat ( sort keys %{ $CLK_CORR{$key_sys} } ) {
        my $colour = sprintf ( "%d", ( substr $key_sat, -2 ) + 1 );
        my $dataset = Chart::Gnuplot::DataSet->new(
                                                    xdata    => $CLK_CORR{$key_sys}{$key_sat}{EPOCH},
                                                    ydata    => $CLK_CORR{$key_sys}{$key_sat}{DATA},
                                                    title    => "$key_sat",
                                                    timefmt  => '%Y-%m-%dT%H:%M:%S',                    # '%H:%M:%S',
                                                    linetype => $colour,
                                                    style    => " linespoints ",
        );
        push ( @datasets, $dataset );
    }
    $chart->plot2d(@datasets);
    $y = $y - $dy;
    if ( $y < 30 / mm ) {
        $page = $pdf->page();
        $page->mediabox('A4');
        $y = $y0;
    }
    $png = $page->gfx();
    die ("Unable to find image file: $!") unless -e $pngName;
    $png->image( $pdf->image_png($pngName), $x, $y, $width, $height );
}

######### CLOCK FULL #####################
print "Plot FULL CLOCK ...\n";
$page = $pdf->page();
$page->mediabox('A4');
$headline = sprintf ( "Full Clock (%s)", $inputFilename );
$headline_text = $page->text;
$headline_text->font( $font1, 11 / pt );
$headline_text->translate( 15 / mm, 280 / mm );
$headline_text->text($headline);
$y = $y0 + $dy;

# SYSTEM
foreach my $key_sys ( sort keys %CLK_FULL ) {
    my @datasets;    # init datasets
    my $pngName = sprintf ( "%s_CLK_FULL_%s.png", $inputFilename, $key_sys );
    my $chart = Chart::Gnuplot->new(
        output   => $pngName,
        terminal => 'png',
        title    => "$key_sys",
        ylabel   => "Full Clock [m]",                             #yrange => [" -3 ", " 3 "],
        xlabel   => "Time [h]",
        timeaxis => 'x',
        xtics    => { labelfmt => '%H:%M', rotate => '-270', },
        legend => { position => "outside below", },
        grid   => 'on',
    );

    # SATELLITE
    foreach my $key_sat ( sort keys %{ $CLK_FULL{$key_sys} } ) {
        my $colour = sprintf ( "%d", ( substr $key_sat, -2 ) + 1 );
        my $dataset = Chart::Gnuplot::DataSet->new(
                                                    xdata    => $CLK_FULL{$key_sys}{$key_sat}{EPOCH},
                                                    ydata    => $CLK_FULL{$key_sys}{$key_sat}{DATA},
                                                    title    => "$key_sat",
                                                    timefmt  => '%Y-%m-%dT%H:%M:%S',                    # '%H:%M:%S',
                                                    linetype => $colour,
                                                    style    => " linespoints ",
        );
        push ( @datasets, $dataset );
    }
    $chart->plot2d(@datasets);
    $y = $y - $dy;
    if ( $y < 30 / mm ) {
        $page = $pdf->page();
        $page->mediabox('A4');
        $y = $y0;
    }
    $png = $page->gfx();
    die ("Unable to find image file: $!") unless -e $pngName;
    $png->image( $pdf->image_png($pngName), $x, $y, $width, $height );
}

######### OFFSET SAT #####################
if ( $plotType eq "KF" ) {
    print "Plot OFFSET SAT ...\n";
    $page = $pdf->page();
    $page->mediabox('A4');
    $headline = sprintf ( "AC and satellite specific offset  (%s)", $inputFilename );
    $headline_text = $page->text;
    $headline_text->font( $font1, 11 / pt );
    $headline_text->translate( 15 / mm, 280 / mm );
    $headline_text->text($headline);
    $y = $y0 + $dy;

    # AC
    foreach my $key_ac ( sort keys %OFFSET_SAT ) {    #print "$key_ac \n";
                                                      #SYSTEM
        foreach my $key_sys ( sort keys %{ $OFFSET_SAT{$key_ac} } ) {
            my @datasets;                             # init datasets
            my $pngName = sprintf ( "%s_OFFSETSAT_%s_%s.png", $inputFilename, $key_ac, $key_sys );
            my $chartOFFSETSAT = Chart::Gnuplot->new(
                output   => $pngName,
                terminal => 'png',
                title    => "$key_ac - $key_sys",
                ylabel   => "AC and satellite specific offset [m]",      # yrange => [" -0.1 ", " 0.1 "],
                xlabel   => "Time [h]",
                timeaxis => 'x',
                xtics    => { labelfmt => '%H:%M', rotate => '-270' },
                legend   => { position => "outside below", },
                grid   => 'on',
            );

            # SATELLITE
            foreach my $key_sat ( sort keys %{ $OFFSET_SAT{$key_ac}{$key_sys} } ) {
                my $colour = sprintf ( "%d", ( substr $key_sat, -2 ) + 1 );
                my $dataset = Chart::Gnuplot::DataSet->new(
                                            xdata    => $OFFSET_SAT{$key_ac}{$key_sys}{$key_sat}{EPOCH},
                                            ydata    => $OFFSET_SAT{$key_ac}{$key_sys}{$key_sat}{DATA},
                                            title    => "$key_sat",
                                            timefmt  => '%Y-%m-%dT%H:%M:%S',                               # '%H:%M:%S',
                                            linetype => $colour,
                                            style    => " linespoints ",
                );
                push ( @datasets, $dataset );
            }
            $chartOFFSETSAT->plot2d(@datasets);
            $y = $y - $dy;
            if ( $y < 30 / mm ) {
                $page = $pdf->page();
                $page->mediabox('A4');
                $y = $y0;
            }
            $png = $page->gfx();
            die ("Unable to find image file: $!") unless -e $pngName;
            $png->image( $pdf->image_png($pngName), $x, $y, $width, $height );
        }
    }
}
######### OFFSET AC #####################
print "Plot OFFSET AC ...\n";
$page = $pdf->page();
$page->mediabox('A4');
$headline = sprintf ( "Common AC offsets (%s)", $inputFilename );
$headline_text = $page->text;
$headline_text->font( $font1, 11 / pt );
$headline_text->translate( 15 / mm, 280 / mm );
$headline_text->text($headline);
$y = $y0 + $dy;

# SYSTEM
foreach my $key_sys ( sort keys %OFFSET_AC ) {
    my @datasets;    # init datasets
    my $pngName = sprintf ( "%s_OFFSETAC_%s.png", $inputFilename, $key_sys );
    my $chart = Chart::Gnuplot->new(
        output   => $pngName,
        terminal => 'png',
        title    => "$key_sys",
        ylabel   => "Common Offset [m]",                         #yrange => ["-20.0", "20.0"],
        xlabel   => "Time [h]",
        timeaxis => 'x',
        xtics    => { labelfmt => '%H:%M', rotate => '-270' },
        legend => { position => "outside below", },
        grid   => 'on',
    );
    # SATELLITE
    foreach my $key_ac ( sort keys %{ $OFFSET_AC{$key_sys} } ) {
        my $dataset = Chart::Gnuplot::DataSet->new(
                                                    xdata   => $OFFSET_AC{$key_sys}{$key_ac}{EPOCH},
                                                    ydata   => $OFFSET_AC{$key_sys}{$key_ac}{DATA},
                                                    title   => "$key_ac",
                                                    timefmt => '%Y-%m-%dT%H:%M:%S',                    # '%H:%M:%S',
                                                    style   => " linespoints ",
        );
        push ( @datasets, $dataset );
    }
    $chart->plot2d(@datasets);
    $y = $y - $dy;
    if ( $y < 30 / mm ) {
        $page = $pdf->page();
        $page->mediabox('A4');
        $y = $y0;
    }
    $png = $page->gfx();
    die ("Unable to find image file: $!") unless -e $pngName;
    $png->image( $pdf->image_png($pngName), $x, $y, $width, $height );
}
######### RES #####################
print "Plot RES ...\n";
$page = $pdf->page();
$page->mediabox('A4');
$headline = sprintf ( "Residuals (%s)", $inputFilename );
$headline_text = $page->text;
$headline_text->font( $font1, 11 / pt );
$headline_text->translate( 15 / mm, 280 / mm );
$headline_text->text($headline);
$y = $y0 + $dy;
$yrange = 5.0;
if ( $plotType eq "KF" ) {
    $yrange = 0.1;
}
# AC
foreach my $key_ac ( sort keys %RES ) {    #print "$key_ac \n";
    #SYSTEM
    foreach my $key_sys ( sort keys %{ $RES{$key_ac} } ) {
        my @datasets;                      # init datasets
        my $pngName = sprintf ( "%s_RES_%s_%s.png", $inputFilename, $key_ac, $key_sys );
        my $chart = Chart::Gnuplot->new(
                                         output   => $pngName,
                                         terminal => 'png',
                                         title    => "$key_ac - $key_sys",
                                         ylabel   => "Residuals [m]",
                                         yrange   => [ " -$yrange ", "$yrange " ],
                                         xlabel   => "Time [h]",
                                         timeaxis => 'x',
                                         xtics    => { labelfmt => '%H:%M', rotate => '-270', },
                                         legend   => { position => "outside below", },
                                         grid     => 'on',
        );
        # SATELLITE
        foreach my $key_sat ( sort keys %{ $RES{$key_ac}{$key_sys} } ) {
            my $colour = sprintf ( "%d", ( substr $key_sat, -2 ) + 1 );
            my $dataset = Chart::Gnuplot::DataSet->new(
                                                   xdata    => $RES{$key_ac}{$key_sys}{$key_sat}{EPOCH},
                                                   ydata    => $RES{$key_ac}{$key_sys}{$key_sat}{DATA},
                                                   title    => "$key_sat",
                                                   timefmt  => '%Y-%m-%dT%H:%M:%S',                        # '%H:%M:%S',
                                                   style    => " linespoints ",
            );
            push ( @datasets, $dataset );
        }
        $chart->plot2d(@datasets);
        $y = $y - $dy;
        if ( $y < 30 / mm ) {
            $page = $pdf->page();
            $page->mediabox('A4');
            $y = $y0;
        }
        $png = $page->gfx();
        die ("Unable to find image file: $!") unless -e $pngName;
        $png->image( $pdf->image_png($pngName), $x, $y, $width, $height );
    }
}

$pdf->save();
$pdf->end();
system ("rm $inputDir/*png");
system("evince $inputDir/$pdf_name&");

#foreach my $t(@array) {print"$t \n ";}
#print Dumper \%AMB;
#########################################
sub HELP_MESSAGE {
    print <<EOI_HILFE;
cmbPlot.pl - Script to pplot BNC's PPP logfiles

CALL:
      cmbPlot.pl [OPTIONS]

OPTIONS:
      --plotType      KF (Kalman Filter combination) or SE (Single Epoch Combination)
      --logFiles      comma separated list of BNC's combination logfiles
      --help          show help contents

DESCRIPTION:

EOI_HILFE
    exit;
}
