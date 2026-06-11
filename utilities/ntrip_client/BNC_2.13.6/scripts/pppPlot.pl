#!/usr/bin/env perl

# ========================================================================
# pppPlot.pl - plot BNC's PPP results using gnuplot
# ========================================================================
#
# Plot metrics:
#    - NEU displacements w.r.t. coordinates in Crd file
#    - A-priori + correction values of tropospheric zenith delay in [m],
#    - Receiver clock error and offsets in [m],
#    - Elevations, given per satellite
#    - Ambiguities, given per satellite
#    - Ionosphere Delay [m],
#    - Receiver Bias [m]
#    - Code and phase residuals in [m],
#
# Author  : Andrea Stuerze
# Revision: $Header: https://software.rtcm-ntrip.org/svn/tags/BNC_2.13.6/scripts/pppPlot.pl 10879 2026-04-14 08:41:02Z stuerze $
# Changes :
# ========================================================================

# Uses
use strict;
use warnings;

BEGIN {
	use FindBin qw($Bin);
	use lib "$Bin";
}

use diagnostics;
use PDL;
use FindBin qw($Bin);
use Getopt::Long;
use Chart::Gnuplot;
use Data::Dumper qw(Dumper);
use File::Basename;
use Date::Manip;
use Log::Log4perl qw(:easy);
use PDF::API2;

use Bnc;
use Common;

use constant {
	mm   => 25.4 / 72,
	inch => 1 / 72,
	pt   => 1,
}; # There are 72 postscript points in an inch and there are 25.4 millimeters in an inch.

# Logging
Log::Log4perl->easy_init(
	{

		#file   => "plotPPP.log",
		layout => '%d [%c l%L] %p: %m%n',
		level  => $TRACE
	}
);

# Options
my ($prog)    = fileparse($0);
my $help      = 0;
my @plotTypes = ();
my @logFiles  = ();
my $sampling  = 1;

GetOptions(
	'help'        => \$help,
	'plotTypes=s' => \@plotTypes,
	'logFiles=s'  => \@logFiles,
	'sampling=s'  => \$sampling,
);

HELP_MESSAGE() if $help;
@plotTypes = qw(NEU) unless (@plotTypes);
@plotTypes = map { uc } split( /[, ]/, join( ',', @plotTypes ) );
@logFiles  = split( /[, ]/, join( ',', @logFiles ) );
unless (@logFiles) { ERROR "logfiles missing"; HELP_MESSAGE() }
DEBUG(
"\n       plotTpes: @plotTypes\n       logfiles: @logFiles\n       sampling:  $sampling"
);

# -----------------------------------------------------------------------------
# Generate data sets for gnuplot
# -----------------------------------------------------------------------------
# for pdf gerneration
my ( $png, $page, $headline, $headline_text );
my $y0 = 180 / mm;
my ( $x, $y, $width, $height ) = ( 40 / mm, $y0, 130 / mm, 80 / mm );
my $dy = $height + 10 / mm;

# Loop over logfiles
foreach my $file (@logFiles) {
	DEBUG "Parse logfile $file";

 # -----------------------------------------------------------------------------
 # Create pdf for plot results
 # -----------------------------------------------------------------------------
	my ( $inputFilename, $inputDir, $inputSuffix ) = fileparse( $file, '\..*' );
	my $pdf_name = sprintf( "%s.pdf", $inputFilename );
	my $pdf      = PDF::API2->new( -file => "$inputDir$pdf_name" );
	my $font1    = $pdf->corefont('Helvetica-Bold');

 # -----------------------------------------------------------------------------
 # Read logfile
 # -----------------------------------------------------------------------------
	my ( $dateStr, $station, $ssrData, $file ) = Bnc::parsePPPLogfile( $file, $sampling );
	# PPP #
	my $EPOCH_PPP = $file->{'EPOCH_PPP'};
	my $N         = $file->{'N'};
	my $E         = $file->{'E'};
	my $U         = $file->{'U'};
	my $ISFIX     = $file->{'ISFIX'};
	my $NUMFIX    = $file->{'NUMFIX'};
	my %RECCLK    = %{ $file->{'RECCLK'} };
	my %AMB       = %{ $file->{'AMB'} };
	my %RES       = %{ $file->{'RES'} };
	my %ELE       = %{ $file->{'ELE'} };
	my %ION       = %{ $file->{'ION'} };
	my %BIA       = %{ $file->{'BIA'} };

	# SSR #
	my $EPOCH_SSR = $file->{'EPOCH_SSR'};
	my %CLKCORR   = %{ $file->{'CLKCORR'} };
	my %YAW_SSR   = %{ $file->{'YAW_SSR'} };
	my %YAW_DEF   = %{ $file->{'YAW_DEF'} };
	my %CODEBIAS  = %{ $file->{'CODEBIAS'} };
	my %PHASEBIAS = %{ $file->{'PHASEBIAS'} };
	my %JUMPCOUNT = %{ $file->{'JUMPCOUNT'}};

 # -----------------------------------------------------------------------------
 # RMS computation
 # -----------------------------------------------------------------------------
	my (
		$mean, $prms,  $median, $min,   $max,
		$adev, $rms_n, $rms_e,  $rms_u, $rms_trp
	);
	my ( $n, $e, $u, $trp, $str_rms_n, $str_rms_e, $str_rms_u, $str_rms_trp );
	$n = pdl( $file->{'N'} );
	( $mean, $prms, $median, $min, $max, $adev, $rms_n ) = stats($n);
	$e = pdl( $file->{'E'} );
	( $mean, $prms, $median, $min, $max, $adev, $rms_e ) = stats($e);
	$u = pdl( $file->{'U'} );
	( $mean, $prms, $median, $min, $max, $adev, $rms_u ) = stats($u);
	$trp = pdl( $file->{'TRP'} );
	( $mean, $prms, $median, $min, $max, $adev, $rms_trp ) = stats($trp);
	$str_rms_n   = sprintf( " %.2f ", $rms_n );
	$str_rms_e   = sprintf( " %.2f ", $rms_e );
	$str_rms_u   = sprintf( " %.2f ", $rms_u );
	$str_rms_trp = sprintf( " %.2f ", $rms_trp );
	DEBUG(
"RMS: North: $str_rms_n, East: $str_rms_e, Up: $str_rms_u, TRP: $str_rms_trp"
	);

 # -----------------------------------------------------------------------------
 # Plot several data sets
 # -----------------------------------------------------------------------------
	my $dataset;
	######### NEU #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		$page = $pdf->page();
	    $page->mediabox('A4');
	    $headline      = sprintf( "PPP results for station %s", $station );
    	$headline_text = $page->text;
	    $headline_text->font( $font1, 11 / pt );
    	$headline_text->translate( 15 / mm, 280 / mm );
	    $headline_text->text($headline);
	    $y = $y0;
		DEBUG "Plot NEU ";
		my $pngNameNEU = sprintf( "%s_NEU.png", $station );
		my $chartNEU   = newChart("$station ($dateStr)");
		$chartNEU->set(
			output => $pngNameNEU,
			ylabel => "Displacements [m]",
			yrange => [ " -0.5 ", " 0.5 " ],
		);
		my $dataN = Chart::Gnuplot::DataSet->new(
			xdata   => $EPOCH_PPP,
			ydata   => $N,
			title   => "Displacements N, RMS + -$str_rms_n m",
			timefmt => '%s',
			style   => "lines",
		);
		my $dataE = Chart::Gnuplot::DataSet->new(
			xdata   => $EPOCH_PPP,
			ydata   => $E,
			title   => "Displacements E, RMS + -$str_rms_e m",
			timefmt => '%s',
			style   => "lines",
		);
		my $dataU = Chart::Gnuplot::DataSet->new(
			xdata   => $EPOCH_PPP,
			ydata   => $U,
			title   => "Displacements U, RMS + -$str_rms_u m",
			timefmt => '%s',
			style   => "dots",
		);
		my @datasets = ( $dataN, $dataE, $dataU );
		$chartNEU->plot2d(@datasets);
		$png = $page->gfx();
		LOGDIE("could not find image file: $!\n") unless -e $pngNameNEU;
		$png->image( $pdf->image_png($pngNameNEU), $x, $y, $width, $height );
	}
	######### FIX #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot FIX";
		my $pngNameFIX = sprintf( "%s_FIX.png", $station );
		my $chartFIX   = newChart("$station ($dateStr)");
		$chartFIX->set(
			output => $pngNameFIX,
			ylabel => "Fixed [%]",
			yrange => [ "0", "100" ],
			ytics  => 20,
		);
		my $dataNUMFIX = Chart::Gnuplot::DataSet->new(
			xdata   => $EPOCH_PPP,
			ydata   => $NUMFIX,
			timefmt => '%s',
			style   => "impulse",
			color   => "green",
		);
		$chartFIX->plot2d($dataNUMFIX);
		$y = $y - $dy;
		if ( $y < 30 / mm ) {
			$page = $pdf->page();
			$page->mediabox('A4');
			$y = $y0;
		}
		$png = $page->gfx();
		LOGDIE("could not find image file: $!\n") unless -e $pngNameFIX;
		$png->image( $pdf->image_png($pngNameFIX), $x, $y, $width, $height );
	}
	######### TRP #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot TRP";
		my $pngNameTRP = sprintf( "%s_TRP.png", $station );
		my $chartTRP   = newChart("$station ($dateStr)");
		$chartTRP->set( output => $pngNameTRP );
		$chartTRP->set(
			ylabel => "Tropospheric Delay [m]",
			yrange => [ " 1.0 ", " 3.0 " ],
		);
		my $dataTRP = Chart::Gnuplot::DataSet->new(
			xdata   => $EPOCH_PPP,
			ydata   => $file->{'TRP'},
			title   => "Tropospheric Delay, RMS + -$str_rms_trp m",
			timefmt => '%s',
			style   => "dots",
		);
		$chartTRP->plot2d($dataTRP);
		$y = $y - $dy;

		if ( $y < 30 / mm ) {
			$page = $pdf->page();
			$page->mediabox('A4');
			$y = $y0;
		}
		$png = $page->gfx();
		LOGDIE("could not find image file: $!\n") unless -e $pngNameTRP;
		$png->image( $pdf->image_png($pngNameTRP), $x, $y, $width, $height );
	}
	######### RECCLK #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot Receiver Clocks";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Receiver Clocks for station %s", $station );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		my $chartRECCLK = newChart("$station ($dateStr)");
		$chartRECCLK->set(
		    legend => { position => "outside right" } ,
		);
		my @datasets      = ();                                  # init datasets
		my $pngNameRECCLK = sprintf( "%s_RECCLK.png", $station );
		$chartRECCLK->set( output => $pngNameRECCLK );
		$chartRECCLK->set( ylabel => "Receiver Clocks [m]" );
		# SYSTEM
		foreach my $key_sys ( sort keys %RECCLK ) {
			$dataset = Chart::Gnuplot::DataSet->new(
				xdata => \@{ $RECCLK{$key_sys}{EPOCH_PPP} },   # array of epochs
				ydata => \@{ $RECCLK{$key_sys}{DATA} }
				,    # array of elevations of one satellite
				title   => "$key_sys",
				timefmt => '%s',
				style   => "dots",
			);
			push( @datasets, $dataset );
		}
		$chartRECCLK->plot2d(@datasets);
		$y = $y - $dy;
		if ( $y < 30 / mm ) {
			$page = $pdf->page();
			$page->mediabox('A4');
			$y = $y0;
		}
		$png = $page->gfx();
		die("could not find image file: $!") unless -e $pngNameRECCLK;
		$png->image( $pdf->image_png($pngNameRECCLK), $x, $y, $width, $height );
	}
	######### ELE #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot Elevations";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y        = $y0 + $dy;
		$headline = sprintf( "Satellite Elevations for station %s", $station );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		my $chartELE = newChart("$station ($dateStr)");
		$chartELE->set(
		    legend => { position => "outside right" },
		);
		# SYSTEM #print Dumper \%ELE;
		foreach my $key_sys ( sort keys %ELE ) {
			# print "$key_sys \n";# print Dumper $ELE{$key_sys};
			my @datasets = ();    # init datasets
			my $pngNameELE = sprintf( "%s_ELE_%s.png", $station, $key_sys );
			$chartELE->set( output => $pngNameELE );
			$chartELE->set(
				ylabel => "Elevation [°]",
				yrange => [ " 0.0 ", " 90.0 " ]
			);
			# SATELLITE
			foreach my $key_sat ( sort keys %{ $ELE{$key_sys} } ) {				# print "$key_sat = $ELE{$key_sys}{$key_sat} \n";
				$dataset = Chart::Gnuplot::DataSet->new(
					xdata => \@{ $ELE{$key_sys}{$key_sat}{EPOCH_PPP} }
					,    # array of epochs
					ydata => \@{ $ELE{$key_sys}{$key_sat}{DATA} }
					,    # array of elevations of one satellite
					title   => "$key_sat",
					timefmt => '%s',
					style   => "dots",
				);
				push( @datasets, $dataset );
			}
			$chartELE->plot2d(@datasets);
			$y = $y - $dy;
			if ( $y < 30 / mm ) {
				$page = $pdf->page();
				$page->mediabox('A4');
				$y = $y0;
			}
			$png = $page->gfx();
			die("could not find image file: $!") unless -e $pngNameELE;
			$png->image( $pdf->image_png($pngNameELE), $x, $y, $width,
				$height );
		}
	}
	######### AMB #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot Ambiguities";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Ambiguities for station %s", $station );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		# AMBIGUITY_TYPE #print Dumper \%AMB;
		foreach my $key_ambType (%AMB) {
			foreach my $key_sys ( sort keys %{ $AMB{$key_ambType} } ) {
				my ( @datasets_amb, @datasets_epo );    # init datasets
				my $pngNameAMB = sprintf( "%s_AMB_%s_%s.png",	$station, $key_ambType, $key_sys );
				my $pngNameEPO = sprintf( "%s_EPO_%s_%s.png",	$station, $key_ambType, $key_sys );
				my $chartAMB = Chart::Gnuplot->new(
					output   => $pngNameAMB,
					terminal => 'png',
					title    => "$station ($dateStr)",
					ylabel   => "Ambiguities $key_ambType [m]",
					timeaxis => 'x',
					xtics    => { labelfmt => '%H:%M', rotate => '-270', },
					legend   => { position => "outside right", },
					grid     => 'on',
				);
				my $chartEPO = Chart::Gnuplot->new(
					output   => $pngNameEPO,
					terminal => 'png',
					title    => "$station ($dateStr)",
					ylabel   => "Number of Epochs $key_ambType [-]",
					timeaxis => 'x',
					xtics    => { labelfmt => '%H:%M', rotate => '-270', },
					legend   => { position => "outside right", },
					grid     => 'on',
				);
				# SATELLITE
				foreach  my $key_sat ( sort keys %{ $AMB{$key_ambType}{$key_sys} } ){
					# ambiguities
					my $dataset_amb = Chart::Gnuplot::DataSet->new(
						xdata =>  $AMB{$key_ambType}{$key_sys}{$key_sat}{EPOCH_PPP}		,    # array of epochs
						ydata => $AMB{$key_ambType}{$key_sys}{$key_sat}{DATA}			,    # array of ambiguities of one satellite
						title   => "$key_sat",
						timefmt => '%s',
						style   => "dots",
					);
					push( @datasets_amb, $dataset_amb );
					# number of epochs used for ambiguity
					my $dataset_epo = Chart::Gnuplot::DataSet->new(
						xdata =>  $AMB{$key_ambType}{$key_sys}{$key_sat}{EPOCH_PPP}	,    # array of epochs
						ydata => $AMB{$key_ambType}{$key_sys}{$key_sat}{NUMEPO}		,    # array of ambiguities of one satellite
						title   => "$key_sat",
						timefmt => '%s',
						style   => "dots",
					);
					push( @datasets_epo, $dataset_epo );
				}
				# ambiguities
				$chartAMB->plot2d(@datasets_amb);
				$y = $y - $dy;
				if ( $y < 30 / mm ) {
					$page = $pdf->page();
					$page->mediabox('A4');
					$y = $y0;
				}
				$png = $page->gfx();
				LOGDIE("could not find image file: $!\n")
				  unless -e $pngNameAMB;
				$png->image( $pdf->image_png($pngNameAMB),	$x, $y, $width, $height );
				# number of epochs used for ambiguity
				$chartEPO->plot2d(@datasets_epo);
				$y = $y - $dy;
				if ( $y < 30 / mm ) {
					$page = $pdf->page();
					$page->mediabox('A4');
					$y = $y0;
				}
				$png = $page->gfx();
				LOGDIE("could not find image file $pngNameEPO: $!\n")
				  unless -e $pngNameEPO;
				$png->image( $pdf->image_png($pngNameEPO),
					$x, $y, $width, $height );
			}
		}
	}
	######### ION #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot ION";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Ionosphere Delay for station %s", $station );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		my $chartION = newChart("$station ($dateStr)");
		$chartION->set( ylabel => "Ionophere Delay [m]" );
		$chartION->set( legend => { position => "outside right" } );
		# SYSTEM
		foreach my $ksys ( sort keys %ION ) {
			my @datasets;    # init datasets
			my $pngNameION = sprintf( "%s_ION_%s.png", $station, $ksys );
			$chartION->set( output => $pngNameION );
			# SATELLITE
			foreach my $sat ( sort keys %{ $ION{$ksys} } ) {
				my $dataset = Chart::Gnuplot::DataSet->new(
					xdata => $ION{$ksys}{$sat}{EPOCH_PPP},    # array of epochs
					ydata => $ION{$ksys}{$sat}{DATA}
					,    # array of ionvations of one satellite
					title   => "$sat",
					timefmt => '%s',
					style   => "dots",
				);
				push( @datasets, $dataset );
			}
			$chartION->plot2d(@datasets);    #system ("display $pngName&");
			$y = $y - $dy;
			if ( $y < 30 / mm ) {
				$page = $pdf->page();
				$page->mediabox('A4');
				$y = $y0;
			}
			$png = $page->gfx();
			die("could not find image file: $!") unless -e $pngNameION;
			$png->image( $pdf->image_png($pngNameION), $x, $y, $width,
				$height );
		}
	}
	######### BIAS #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot BIAS";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Receiver Biases for station %s", $station );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		my $chartBIAS = newChart("$station ($dateStr)");
		$chartBIAS->set( legend => { position => "outside right" } );

		# BIAS_TYPE   #print Dumper \%BIA;
		foreach my $key_biasType ( sort keys %BIA )	{    #print "key_biasType: $key_biasType \n";
			foreach my $key_sys ( sort keys %{ $BIA{$key_biasType} } )	{  #print "key_sys: $key_sys \n"; #print Dumper $BIA{$key_biasType};
				my @datasets;    # init datasets
				my $pngNameBIA = sprintf( "%s_BIAS_%s_%s.png",
					$station, $key_biasType, $key_sys );
				$chartBIAS->set( output => $pngNameBIA );
				$chartBIAS->set( ylabel => "Receiver Bias $key_biasType [m]" );

				my $dataset = Chart::Gnuplot::DataSet->new(
					xdata   => $BIA{$key_biasType}{$key_sys}{EPOCH_PPP},
					ydata   => $BIA{$key_biasType}{$key_sys}{DATA},
					title   => "$key_sys",
					timefmt => '%s',
					style   => "dots",
				);
				push( @datasets, $dataset );

				$chartBIAS->plot2d(@datasets);
				$y = $y - $dy;
				if ( $y < 30 / mm ) {
					$page = $pdf->page();
					$page->mediabox('A4');
					$y = $y0;
				}
				$png = $page->gfx();
				die("could not find image file: $!") unless -e $pngNameBIA;
				$png->image( $pdf->image_png($pngNameBIA), $x, $y, $width, $height );
			}
		}
	}
	######### RES #####################
	if ( grep ( $_ eq "PPP", @plotTypes ) ) {
		DEBUG "Plot Residuals";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Residuals for station %s", $station );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		my $chartRES = newChart("$station ($dateStr)");
		$chartRES->set( legend => { position => "outside right" } );
		# RESIDUAL_TYPE   #print Dumper \%RES;
		foreach my $key_resType ( sort keys %RES ){    #print "key_resType: $key_resType \n";
			#SYSTEM
			foreach my $key_sys ( sort keys %{ $RES{$key_resType} } ) {				#print "key_sys: $key_sys \n"; #print Dumper $RES{$key_resType};
				my @datasets;
				my $pngNameRES = sprintf( "%s_RES_%s_%s.png", $station, $key_resType, $key_sys );
				$chartRES->set( output => $pngNameRES );
				$chartRES->set(ylabel => "Residuals $key_resType [m]" );
				if ( $key_resType =~ /^c/ ) {
					$chartRES->set( yrange => [ " -6.0 ", " 6.0 " ] );
				}
				elsif ( $key_resType =~ /^l/ ) {
					$chartRES->set( yrange => [ " -0.06 ", " 0.06 " ] );
				}
				elsif ( $key_resType =~ /^GIM/ ) {
					$chartRES->set( yrange => [ " -6.0 ", " 6.0 " ] );
				}
				# SATELLITE
				foreach my $key_sat ( sort keys %{ $RES{$key_resType}{$key_sys} } ) {			  #print "$key_sat = $RES{$key_resType}{$key_sys}{$key_sat} \n";
					$dataset = Chart::Gnuplot::DataSet->new(
						xdata => $RES{$key_resType}{$key_sys}{$key_sat}{EPOCH_PPP},    # array of epochs
						ydata => $RES{$key_resType}{$key_sys}{$key_sat}{DATA}
						,    # array of residuals of one satellite
						title   => "$key_sat",
						timefmt => '%s',
						style   => "dots",
					);
					push( @datasets, $dataset );
				}
				$chartRES->plot2d(@datasets);
				$y = $y - $dy;
				if ( $y < 30 / mm ) {
					$page = $pdf->page();
					$page->mediabox('A4');
					$y = $y0;
				}
				$png = $page->gfx();
				LOGDIE("could not find image file: $!\n")
				  unless -e $pngNameRES;
				$png->image( $pdf->image_png($pngNameRES), $x, $y, $width, $height );
			}
		}
	}
    ######### CLKCORR #####################
	if ( grep ( $_ eq "SSR", @plotTypes ) ) {
		DEBUG "Plot CLKCORR";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Clock Corrections from %s", $ssrData );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		my $chartCLKCORR = newChart("$ssrData ($dateStr)");
		$chartCLKCORR->set( ylabel => "Clock Corrections [m]" );
		$chartCLKCORR->set( legend => { position => "outside right" } );
		# SYSTEM
		foreach my $ksys ( sort keys %CLKCORR )	{    #print "$key_sys \n"; #print Dumper $CLKCORR{$key_sys};
			my @datasets;
			my $pngNameCLKCORR = sprintf( "%s_CLKCORR_%s.png", $station, $ksys );
			$chartCLKCORR->set( output => $pngNameCLKCORR );
			# SATELLITE
			foreach my $sat ( sort keys %{ $CLKCORR{$ksys} } ) {    #print "$key_sat = $CLKCORR{$key_sys}{$key_sat} \n";
				my $dataset = Chart::Gnuplot::DataSet->new(
					xdata => $CLKCORR{$ksys}{$sat}{EPOCH_SSR}, # array of epochs
					ydata => $CLKCORR{$ksys}{$sat}{DATA},    # array of ionvations of one satellite
					title   => "$sat",
					timefmt => '%s',
					style   => "dots",
				);
				push( @datasets, $dataset );
			}
			$chartCLKCORR->plot2d(@datasets);    #system ("display $pngName&");
			$y = $y - $dy;
			if ( $y < 30 / mm ) {
				$page = $pdf->page();
				$page->mediabox('A4');
				$y = $y0;
			}
			$png = $page->gfx();
			die("could not find image file: $!") unless -e $pngNameCLKCORR;
			$png->image( $pdf->image_png($pngNameCLKCORR),
				$x, $y, $width, $height );
		}
	}
	######### YAW ANGLE #####################
	if ( grep ( $_ eq "SSR", @plotTypes ) ) {
		DEBUG "Plot YAW";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y = $y0 + $dy;
		$headline =
		  sprintf( "Yaw angle from %s versus Standard Model", $ssrData );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		# SATELLITE
		foreach my $sat ( sort keys %YAW_SSR ) {
			my @datasets;
			my $chartYAW = newChart("Yaw Angle Satellite $sat ($dateStr)");
			$chartYAW->set( ylabel => "Yaw Angle [°]" );
			$chartYAW->set( legend => { position => "outside right" } );
			my $pngNameYAW = sprintf( "%s_YAW_%s.png", $station, $sat );
			$chartYAW->set( output => $pngNameYAW );
			# YAW from SSR data
			my $datasetYAW_SSR = Chart::Gnuplot::DataSet->new(
				xdata => $YAW_SSR{$sat}{EPOCH_SSR},    # array of epochs
				ydata => $YAW_SSR{$sat}{DATA}	,    # array of ionvations of one satellite
				title   => $ssrData,
				timefmt => '%s',
				style   => "line",
			);
			push( @datasets, $datasetYAW_SSR );
			# YAW from standard model
			my $datasetYAW_DEF = Chart::Gnuplot::DataSet->new(
				xdata => $YAW_DEF{$sat}{EPOCH_SSR},    # array of epochs
				ydata => $YAW_DEF{$sat}{DATA}	,    # array of ionvations of one satellite
				title   => "Standard Model",
				timefmt => '%s',
				style   => "line",
			);
			push( @datasets, $datasetYAW_DEF );
			$chartYAW->plot2d(@datasets);
			$y = $y - $dy;
			if ( $y < 30 / mm ) {
				$page = $pdf->page();
				$page->mediabox('A4');
				$y = $y0;
			}
			$png = $page->gfx();
			die("could not find image file: $!") unless -e $pngNameYAW;
			$png->image( $pdf->image_png($pngNameYAW), $x, $y, $width,	$height );
		}
	}
	######### CODE BIASES #####################
	if ( grep ( $_ eq "SSR", @plotTypes ) ) {
		DEBUG "Plot Code Biases";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Code Biases from %s ", $ssrData );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		my $chartCB = newChart("$ssrData ($dateStr)");
		$chartCB->set( legend => { position => "outside right" } );
		foreach my $key_type ( sort keys %CODEBIAS ) {
			#SYSTEM
			foreach my $key_sys ( sort keys %{ $CODEBIAS{$key_type} } ) {
				my @datasets;
				my $pngNameCB =  sprintf( "%s_CB_%s_%s.png", $station, $key_type, $key_sys );
				$chartCB->set( output => $pngNameCB );
				$chartCB->set( ylabel => "Code Biases $key_type [m]" );
				# SATELLITE
				foreach  my $key_sat ( sort keys %{ $CODEBIAS{$key_type}{$key_sys} } )	{
					$dataset = Chart::Gnuplot::DataSet->new(
						xdata => $CODEBIAS{$key_type}{$key_sys}{$key_sat}{EPOCH_SSR},    # array of epochs
						ydata => $CODEBIAS{$key_type}{$key_sys}{$key_sat}{DATA}		,    # array of residuals of one satellite
						title   => "$key_sat",
						timefmt => '%s',
						style   => "dots",
					);
					push( @datasets, $dataset );
				}
				$chartCB->plot2d(@datasets);
				$y = $y - $dy;
				if ( $y < 30 / mm ) {
					$page = $pdf->page();
					$page->mediabox('A4');
					$y = $y0;
				}
				$png = $page->gfx();
				LOGDIE("could not find image file: $!\n")
				  unless -e $pngNameCB;
				$png->image( $pdf->image_png($pngNameCB),$x, $y, $width, $height );
			}
		}
	}
	######### PHASE BIASES #####################
	if ( grep ( $_ eq "SSR", @plotTypes ) ) {
		DEBUG "Plot Phase Biases";
		$page = $pdf->page();
		$page->mediabox('A4');
		$y             = $y0 + $dy;
		$headline      = sprintf( "Phase Biases from %s ", $ssrData );
		$headline_text = $page->text;
		$headline_text->font( $font1, 11 / pt );
		$headline_text->translate( 15 / mm, 280 / mm );
		$headline_text->text($headline);
		foreach my $key_type ( sort keys %PHASEBIAS ) {
			#SYSTEM
			foreach my $key_sys ( sort keys %{ $PHASEBIAS{$key_type} } ) {
				my (@datasetsPB, $datasetPB);
				my (@datasetsJC, $datasetJC);
				my $pngNamePB =  sprintf( "%s_PB_%s_%s.png", $station, $key_type, $key_sys );
				my $pngNameJC =  sprintf( "%s_JC_%s_%s.png", $station, $key_type, $key_sys );
				my $chartPB = newChart("$ssrData ($dateStr)");
	        	$chartPB->set( legend => { position => "outside right" } );
				$chartPB->set( output => $pngNamePB );
				$chartPB->set( ylabel => "Phase Biases $key_type [m]" );
				my $chartJC = newChart("$ssrData ($dateStr)");
	        	$chartJC->set( legend => { position => "outside right" } );
				$chartJC->set( output => $pngNameJC );
				$chartJC->set( ylabel => "Counter Value $key_type " );
				# SATELLITE
				foreach  my $key_sat ( sort keys %{ $PHASEBIAS{$key_type}{$key_sys} } )	{
					$datasetPB = Chart::Gnuplot::DataSet->new(
						xdata => $PHASEBIAS{$key_type}{$key_sys}{$key_sat}{EPOCH_SSR},    # array of epochs
						ydata => $PHASEBIAS{$key_type}{$key_sys}{$key_sat}{DATA},    # array of residuals of one satellite
						title   => "$key_sat",
						timefmt => '%s',
						style   => "linespoints",
					);
					push( @datasetsPB, $datasetPB );
					$datasetJC = Chart::Gnuplot::DataSet->new(
						xdata => $JUMPCOUNT{$key_type}{$key_sys}{$key_sat}{EPOCH_SSR},    # array of epochs
						ydata => $JUMPCOUNT{$key_type}{$key_sys}{$key_sat}{DATA},    # array of residuals of one satellite
						title   => "$key_sat",
						timefmt => '%s',
						style   => "linespoints",
					);
					push( @datasetsJC, $datasetJC );
				}
				#phase biases
				$chartPB->plot2d(@datasetsPB);
				$y = $y - $dy;
				if ( $y < 30 / mm ) {
					$page = $pdf->page();
					$page->mediabox('A4');
					$y = $y0;
				}
				$png = $page->gfx();
				LOGDIE("could not find image file: $!\n")
				  unless -e $pngNamePB;
				$png->image( $pdf->image_png($pngNamePB),$x, $y, $width, $height );
				#jump counter
				$chartJC->plot2d(@datasetsJC);
				$y = $y - $dy;
				if ( $y < 30 / mm ) {
					$page = $pdf->page();
					$page->mediabox('A4');
					$y = $y0;
				}
				$png = $page->gfx();
				LOGDIE("could not find image file: $!\n")
				  unless -e $pngNameJC;
				$png->image( $pdf->image_png($pngNameJC),$x, $y, $width, $height );
			}
		}
	}

$pdf->save();
$pdf->end();

system("rm *.png");
if (Common::amInteractiv) {
	system("evince $inputDir/$pdf_name");
}

}    # -----  next logfile  -----

# newChart returns a default chart.
sub newChart {
	my $title = shift;
	return Chart::Gnuplot->new(
		terminal => 'png',
		title    => $title,
		timeaxis => 'x',
        xlabel   => "Time [h]",
		xtics    => { labelfmt => '%H:%M', rotate => '-270' },
		#xtics    => { labelfmt => '%H:%M\n%m-%d', rotate => '-270'},
		grid     => 'on',
	);
}

sub HELP_MESSAGE {
	print <<EOI_HILFE;
$prog - plot BNC's PPP results using gnuplot

USAGE:
  $prog [OPTIONS]

OPTIONS:
  -p, --plotTypes    PPP or SSR (requires PPP files in debug mode)
  -l, --logFiles     comma separated list of BNC's PPP logfiles
  -s, --sampling     sampling interval in seconds for the logfile data (default: 1); for a daily logfile <30> seconds should be usefull
  -h, --help         show help contents

EXAMPLES:
  $prog --plotTypes PPP -s 30 -l /path/to/FFMJ186730.ppp

2021 andrea.stuerze\@bkg.bund.de
EOI_HILFE
	exit;
}
