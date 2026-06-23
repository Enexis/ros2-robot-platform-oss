#!/usr/bin/env perl
# ========================================================================
# compareSP3Plot.pl
# ========================================================================
#
# Purpose: Script using gnuplot to plot BNC's SP3 Comparison Results
#
# Comment: stored values:
# To compare satellite clocks provided by the two files, BNC first converts
# coordinate differences dX,dY,dZ into along track, out-of-plane, and radial
# components.
# It then corrects the clock differences for the radial components of
# coordinate differences. RMS values of clock differences are
# finally calculated after introducing
# - at first one offset 'per epoch for all satellites' and
# - secondly one offset 'per satellite for all epochs'.
# ========================================================================
# Uses
use strict;
use warnings;
use Getopt::Long;
use Chart::Gnuplot;
use Data::Dumper qw(Dumper);
use diagnostics;
use File::Basename;
use Date::Manip;
use Time::Piece 1.30;
use List::MoreUtils qw( minmax );
use experimental 'smartmatch';

# -----------------------------------------------------------------------------
# required to create a pdf
# -----------------------------------------------------------------------------
use PDF::API2;
use constant {mm   => 25.4 / 72, inch => 1 / 72,pt   => 1,
};# There are 72 postscript points in an inch and there are 25.4 millimeters in an inch.
# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
# init options
my $help      = 0;
my @plotTypes = ();
my @logFiles  = ();
# read options from command line
GetOptions(
    'help'        => \$help,
    'plotTypes=s' => \@plotTypes,
    'logFiles=s'  => \@logFiles,
);
help() if $help;
@plotTypes = qw(NEU) unless (@plotTypes);# Default values
@plotTypes = map {uc} split ( /[, ]/, join ( ',', @plotTypes ) );# Reformat options
@logFiles  = split ( /[, ]/, join ( ',', @logFiles ) );
unless (@logFiles) {
    print "ERROR: logfiles missing\n";
    exit;
}
print "       plotTpes: @plotTypes\n       logfiles: @logFiles\n\n";
# -----------------------------------------------------------------------------
# generate data sets for gnuplot
# -----------------------------------------------------------------------------
my $epoSec = 0;
# for pdf generation
my ($png, $page, $headline, $headline_text);
my $y0 = 190/mm;
my ($x, $y, $width, $height) = (40/mm, $y0, 130/mm, 70/mm);
my $dy = $height+7/mm;
# file by file ..
foreach my $file (@logFiles) {
    my (%COMPARISON);
    my (@array);
    my ($sys, $sat, $epo, $radial, $along, $out, $drange, $clk, $clkRed);
    # -----------------------------------------------------------------------------
    # create pdf for plot results
    # -----------------------------------------------------------------------------
    my($inputFilename, $inputDir, $inputSuffix) = fileparse($file, '\..*');
    my $pdf_name = sprintf("%s.pdf", $inputFilename);
    my $pdf = PDF::API2->new(-file => "$inputDir$pdf_name");
    my $font1 = $pdf->corefont('Helvetica-Bold');
    # -----------------------------------------------------------------------------
    # read file and save data
    # -----------------------------------------------------------------------------
    open ( INPUT, "<$file" ) || die $!;
    print "Parse logfile $file ...\n";
    while (<INPUT>) {#56985.000000 G01  0.0160 -0.0242  0.0961     0.0082 -0.0077     1
        if ( $_ !~ /^!/) { #print "$_\n";
            @array = split(/\s+/, $_);
            $epo   = $array[0];   #print "$epo\n";
            $epo   =~s/(\.\d*)//; #print "$epo\n";
            $sys    = substr $array[1], 0, 1;
            $sat    = $array[1];#print "$sat\n";
            $radial = $array[2];#print "$radial\n";
            $along  = $array[3];#print "$along\n";
            $out    = $array[4];#print "$out\n";
            $clk    = $array[5];#print "$clk\n";
            $clkRed = $array[6];#print "$clkRed\n";
            if ($out !~ /^\.$/) {
              push @{$COMPARISON{$sys}{$sat}{EPOCH }}, $epo;
              push @{$COMPARISON{$sys}{$sat}{RADIAL}}, $radial;
              push @{$COMPARISON{$sys}{$sat}{ALONG }}, $along;
              push @{$COMPARISON{$sys}{$sat}{OUT   }}, $out;
              push @{$COMPARISON{$sys}{$sat}{CLK   }}, $clk;
              push @{$COMPARISON{$sys}{$sat}{CLKRED}}, $clkRed;
            }
        }
    }
    close INPUT;

    # -----------------------------------------------------------------------------
    # plot several data sets
    # -----------------------------------------------------------------------------
    ######### RAO #####################
    if (("ALL" ~~ @plotTypes ) || ("RAO" ~~ @plotTypes)) {
        print "Plot RADIAL, ALONG TRACK and OUT OF PLANE COMPONENTS ...\n";
        my $yrange = .6;
        $page = $pdf->page(); $page->mediabox('A4');
        $headline = sprintf("Orbit Differences in radial, along-track and out-of-plane component (%s)", $inputFilename);
        $headline_text = $page->text;
        $headline_text->font($font1, 11/pt);
        $headline_text->translate(15/mm, 280/mm);
        $headline_text->text($headline);
        $y = $y0+$dy;
        #SYSTEM
        foreach my $key_sys (sort keys %COMPARISON) {
            print "    for $key_sys\n";
            my (@datasets_r, @datasets_a, @datasets_o); # init datasets
            my $pngName_r   = sprintf ("%s_RADIAL_%s.png", $inputFilename, $key_sys);
            my $pngName_a   = sprintf ("%s_ALONG_%s.png", $inputFilename, $key_sys);
            my $pngName_o   = sprintf ("%s_OUT_%s.png", $inputFilename, $key_sys);
            my $chart_r = Chart::Gnuplot->new(
                          output => $pngName_r, terminal => 'png', title => "Radial component",
                          ylabel => "Orbit Difference [m]", yrange => ["-$yrange", "$yrange"],
                          xlabel => "Time [h]", timeaxis => 'x', xtics => {labelfmt => '%H:%M', rotate => '-270',},
                          legend => { position => "outside right",},
                          grid => 'on',);
            my $chart_a = Chart::Gnuplot->new(
                          output => $pngName_a, terminal => 'png', title => "Along track component",
                          ylabel => "Orbit Difference [m]", yrange => ["-$yrange", "$yrange"],
                          xlabel => "Time [h]", timeaxis => 'x', xtics => {labelfmt => '%H:%M', rotate => '-270',},
                          legend => { position => "outside right",},
                          grid => 'on',);
            my $chart_o = Chart::Gnuplot->new(
                          output => $pngName_o, terminal => 'png', title => "Out of plane component",
                          ylabel => "Orbit Difference [m]", yrange => ["-$yrange", "$yrange"],
                          xlabel => "Time [h]", timeaxis => 'x', xtics => {labelfmt => '%H:%M', rotate => '-270',},
                          legend => { position => "outside right",},
                          grid => 'on',);
            #SATELLITE
            foreach my $key_sat (sort keys %{$COMPARISON{$key_sys}}) {
                my $colour = sprintf("%d", (substr $key_sat, -2)+1);
                #RADIAL
                my $dataset_r = Chart::Gnuplot::DataSet->new(
                                xdata => \@{$COMPARISON{$key_sys}{$key_sat}{EPOCH}},
                                ydata => \@{$COMPARISON{$key_sys}{$key_sat}{RADIAL}},
                                title => "$key_sat",
                                timefmt => '%Y-%m-%d_%H:%M:%S',
                                linetype => $colour,
                                #style => " linespoints ",);
                                style => " lines ",);
                push (@datasets_r, $dataset_r);
                # ALONG TRACK
                my $dataset_a = Chart::Gnuplot::DataSet->new(
                                xdata => \@{$COMPARISON{$key_sys}{$key_sat}{EPOCH}},
                                ydata => \@{$COMPARISON{$key_sys}{$key_sat}{ALONG}},
                                title => "$key_sat",
                                timefmt => '%Y-%m-%d_%H:%M:%S',
                                linetype => $colour,
                                style => " lines ",);
                push (@datasets_a, $dataset_a);
                # OUT OF PLANE
                my $dataset_o = Chart::Gnuplot::DataSet->new(
                                xdata => \@{$COMPARISON{$key_sys}{$key_sat}{EPOCH}},
                                ydata => \@{$COMPARISON{$key_sys}{$key_sat}{OUT}},
                                title => "$key_sat",
                                timefmt => '%Y-%m-%d_%H:%M:%S',
                                linetype => $colour,
                                style => " lines",);
                push (@datasets_o, $dataset_o);
            }
            $chart_r->plot2d(@datasets_r);
            $chart_a->plot2d(@datasets_a);
            $chart_o->plot2d(@datasets_o);
            # RADIAL
            $y = $y - $dy;
            if($y < 30/mm) {
                $page = $pdf->page(); $page->mediabox('A4');
                $y = $y0;
            }
            $png = $page->gfx(); die("Unable to find image file: $!") unless -e $pngName_r;
            $png->image($pdf->image_png($pngName_r), $x, $y, $width, $height);
            # ALONG
            $y = $y - $dy;
            if($y < 30/mm) {
                $page = $pdf->page(); $page->mediabox('A4');
                $y = $y0;
            }
            $png = $page->gfx(); die("Unable to find image file: $!") unless -e $pngName_a;
            $png->image($pdf->image_png($pngName_a), $x, $y, $width, $height);
            # OUT
            $y = $y - $dy;
            if($y < 30/mm) {
                $page = $pdf->page(); $page->mediabox('A4');
                $y = $y0;
            }
            $png = $page->gfx(); die("Unable to find image file: $!") unless -e $pngName_o;
            $png->image($pdf->image_png($pngName_o), $x, $y, $width, $height);
        }
    }
    ######## CLK / CLKRED #####################
    if (("ALL" ~~ @plotTypes ) || ("CLK" ~~ @plotTypes)) {
    	print "Plot CLOCK and CLOCK REDUCED BY RADIAL COMPONENT ...\n";
        my $yrange = 0.6;
        $page = $pdf->page(); $page->mediabox('A4');
        $headline = sprintf("Clock residuals and reduced clock residuals(%s)", $inputFilename);
        $headline_text = $page->text;
        $headline_text->font($font1, 11/pt);
        $headline_text->translate(15/mm, 280/mm);
        $headline_text->text($headline);
        $y = $y0+$dy;
        #SYSTEM
        foreach my $key_sys (sort keys %COMPARISON) {
        	print "    for $key_sys\n";
            my (@datasets_c, @datasets_r); # init datasets
            my $pngName_c  = sprintf ("%s_CLK_%s.png", $inputFilename, $key_sys);
            my $pngName_r  = sprintf ("%s_CLKRED_%s.png", $inputFilename, $key_sys);
            my $chart_c = Chart::Gnuplot->new(
                          output => $pngName_c, terminal => 'png',title => "Clock only",
                          ylabel => "Clock Residuals [m]", yrange => ["-$yrange", "$yrange"],
                          xlabel => "Time [h]", timeaxis => 'x', xtics => {labelfmt => '%H:%M', rotate => '-270',},
                          legend => { position => "outside right",},
                          grid => 'on',);
            my $chart_r = Chart::Gnuplot->new(
                          output => $pngName_r, terminal => 'png',title => "Clock minus radial component",
                          ylabel => "Clock Residuals [m]", yrange => ["-$yrange", "$yrange"],
                          xlabel => "Time [h]", timeaxis => 'x', xtics => {labelfmt => '%H:%M', rotate => '-270',},
                          legend => { position => "outside right",},
                          grid => 'on',);
            #SATELLITE
            foreach my $key_sat (sort keys %{$COMPARISON{$key_sys}}) {
                my $colour = sprintf("%d", (substr $key_sat, -2)+1);
                # CLK
                my $dataset_c = Chart::Gnuplot::DataSet->new(
                                xdata => \@{$COMPARISON{$key_sys}{$key_sat}{EPOCH}},
                                ydata => \@{$COMPARISON{$key_sys}{$key_sat}{CLK}},
                                title => "$key_sat",
                                timefmt => '%Y-%m-%d_%H:%M:%S',
                                linetype => $colour,
                                style => " lines ",);
                push (@datasets_c, $dataset_c);
                my ( $min, $max ) = minmax @{$COMPARISON{$key_sys}{$key_sat}{CLK}};
                if ($max > 1.0) {print "$key_sat: max clk = $max\n"};
                # CLKRED
                my $dataset_r = Chart::Gnuplot::DataSet->new(
                                xdata => \@{$COMPARISON{$key_sys}{$key_sat}{EPOCH}},
                                ydata => \@{$COMPARISON{$key_sys}{$key_sat}{CLKRED}},
                                title => "$key_sat",
                                timefmt => '%Y-%m-%d_%H:%M:%S',
                                linetype => $colour,
                                style => " lines ",);
                push (@datasets_r, $dataset_r);
            }
            $chart_c->plot2d(@datasets_c);
            $chart_r->plot2d(@datasets_r);
            # CLK
            $y = $y - $dy;
            if($y < 30/mm) {
                $page = $pdf->page(); $page->mediabox('A4');
                $y = $y0;
            }
            $png = $page->gfx(); die("Unable to find image file: $!") unless -e $pngName_c;
            $png->image($pdf->image_png($pngName_c), $x, $y, $width, $height);
            # CLKRED
            $y = $y - $dy;
            if($y < 30/mm) {
                $page = $pdf->page(); $page->mediabox('A4');
                $y = $y0;
            }
            $png = $page->gfx(); die("Unable to find image file: $!") unless -e $pngName_r;
            $png->image($pdf->image_png($pngName_r), $x, $y, $width, $height);
        }
    }
    $pdf->save();
    $pdf->end();
    system("rm *png");
    #system("evince $inputDir/$pdf_name&");
}


#########################################
sub help {
     print <<EOI_HILFE;
CALL:
      compareSp3Plot.pl [paramter]

PARAMETER:

      --plotTypes     possible: ALL => RAO and CLOCK; CLK => CLOCK and CLOCKRED; RAO => RAO only
      --logFiles      comma separated list of BNC's combination logfiles
      --help          show help contents

DESCRIPTION:

      Script to plot BNC's SP3 Comparison Results

EOI_HILFE
exit;
}
