package Bnc;

# Perl utility functions for BNC
#
# Revision: $Header: https://software.rtcm-ntrip.org/svn/tags/BNC_2.13.6/scripts/Bnc.pm 10809 2026-02-09 09:45:42Z stuerze $

use strict;
use warnings;
use File::Basename;
use File::Spec::Functions qw(catfile);
use File::Temp qw(tempfile);
use Exporter;
use Time::Piece 1.30;
use PDL::Lite;    # to avoid namespace pollution
use PDL::Primitive;
use Log::Log4perl qw(:easy);

# use List::MoreUtils qw(uniq); # Prototype mismatch with PDL (uniq)

# =============================================================================
# callBnc ($bnc_ini, %$opts_ref)
# =============================================================================
# Call BNC
#
# Param  : $bnc_ini  [optional] The BNC config file that should be used. If not set, the default config is ised.
#          $opts_ref [required] Hash with BNC config options as key. They will overwrite the settings from the config file.
# Return : BNC exit status
# =============================================================================
sub callBnc {
	my ( $bnc, $bnc_ini, $opts ) = @_;

	my $config_file = "";
	if ( $bnc_ini && -s $bnc_ini ) {
		$config_file = "--conf $bnc_ini";
	}
	else {
		DEBUG("callBnc: Use default bnc-ini file");
	}

	my $opts_str = "";
	if ($opts) { $opts_str = options2string($opts) }

	#    my @cmd = (
	#                'xvfb-run',
	#                "--server-args='-screen 0, 1280x1024x8'",    # 1024x768x24
	#                "$bnc",
	#                "--nw",
	#                $opts_str,
	#    );

#my $rc = call_system("xvfb-run -a -e /home/user/xvfb.err --server-args='-screen 0 1280x1024x8' $bnc --nw $config_file $opts_str");
	return Common::runCmd("$bnc --nw $config_file $opts_str");
}

# Converts options map to string used for calling BNC.
sub options2string {
	my ($opts) = shift;

	my $opts_str = "";
	if ($opts) {
		foreach my $key ( keys %{$opts} ) {
			$opts_str .= "--key $key" . ' "' . $opts->{$key} . '" ';
		}
	}
	return $opts_str;
}

# =============================================================================
# parseMessageTypesFromFile
# =============================================================================
# Parse Message-Types with repetition rate from a BNC/scanRTCM logfile
#
# Param  : $logfile     [required] Path of the BNC/scanRTCM logfile
#          $caAbbr      [optional] caster name or abbreviation
#          $tmpPath     [optional] write working files to this path
#
# Return : Message-types for each mountpoint (as Hash-Ref)
#          Hash with mp as key and the list of messTypes as value
#          Example: $VAR1 = { 'RIO10' => ['1004(1)','1006(15)','1008(15)',...],
#                             'CLIB0' => ['1004(1)','1006(10)','1008(10)',...],
#                           };
# =============================================================================
sub parseMessageTypesFromFile {
	my ( $logfile, $caAbbr, $mytmpPath ) = @_;

	unless ( -s $logfile ) {
		ERROR "File [$logfile] is empty or does not exist";
		return;
	}

	my $tmp = File::Temp->new( UNLINK => 1, SUFFIX => '.messtyps' );
	my $casterMessTypeFile = $tmp->filename;

	#my $caster = $logfilename =~ s/\.log\.messtyps//r;
	#INFO "Process caster '$caster'";

	INFO "Process $logfile";

# scan beginns with that line: 16-02-18 23:58:02 WTZ37: Get data in RTCM 3.x format

# First grep for message type lines in bnc-logfile and write them to local temp. directory
# NOTE: 'sort -u' because Ephemeries message-types 1019 (GPS), 1020 (GLONASS), 1045 (Galileo) come
#       for every sat. at the same time/second. This is done for getting the repetition rate
	system(
"grep \"Received message type\" $logfile | sort -u > $casterMessTypeFile"
	  ) == 0
	  or ERROR "Fehler: $!";

	# --------------------------------------------
	# Get message types foreach station/mountpoint
	# --------------------------------------------
	my $cmd = "cat $casterMessTypeFile | awk '{print \$3, \$7}' | sort -u";
	my @messTypes = `$cmd`;    # ['DARX1: 1004\n','DARX1: 1006\n',... ]
	if ( scalar @messTypes < 1 ) {
		ERROR "Could not retrieve message types from file $casterMessTypeFile";
		return;
	}

# Note: Skip first 1500 lines because BNC is weird here, all lines with same timestamp, buffer problem?

# ------------------------------------------------------
# Guess repetition rate for each mountpoint/message-type
# ------------------------------------------------------
# For that get the first $minSamples appearances of mountp with same message type
# and build the differences between them
	my $minSamples = 4;
	foreach my $mpType (@messTypes) {
		chomp $mpType;    # 'AUBG3: 1004'
		my ( $mp, $mt ) = split( /:\s*/, $mpType );
		my $cmd  = "grep \"$mp: Received message type $mt\" $logfile";
		my @rows = `$cmd`;
		if ( scalar @rows < 1 ) {
			ERROR
"Could not retrieve message types for $mpType from file $casterMessTypeFile";
			next;
		}

		if ( scalar @rows <= $minSamples ) {
			WARN
			  "Could not guess repetition rate for message type $mpType: only "
			  . scalar @rows
			  . " matches found";
			next;
		}

		my $repRate = _computeMessTypRepetitonRate( \@rows, $caAbbr );
		if ($repRate) {
			$mpType .= '(' . $repRate . ')';
		}
	}    # -----  end foreach messageType -----

#if ( unlink $casterMessTypeFile ) { TRACE "Removed file [$casterMessTypeFile]" }

	# Return an HASH OF ARRAYS with mp as key and the list of messTypes as value
	my %messTypesHash;
	foreach (@messTypes) {
		my @ele = split( ': ', $_ );
		my $mp  = shift @ele;
		push( @{ $messTypesHash{$mp} }, shift @ele );
	}

	return \%messTypesHash;
}

# =============================================================================
# _computeMessTypRepetitonRate
# =============================================================================
# Guess repetition rate of message types
#
# Param  : $firstMatches [required] Array-Ref with first appearances of station
#          and mess type. Complete logfile lines, e.g.
#          '13-07-04 17:38:26 BRUX0: Received message type 1004 '
#          $caAbbr       [optional] caster abbreviation
# Return : repetition rate in secs, (median value)
# =============================================================================
sub _computeMessTypRepetitonRate {
	my ( $rows, $caAbbr ) = @_;

	my $maxGap = 600;    # if gap > 10min then we guess it is a new scan

	$rows->[0] =~ /.{18}([a-x0-9]+): Received message type (\d+)/i;
	my $stat  = $1;
	my $mestp = $2;

	# Create list of unix timestamps
	my @scans;
	my $scan = 0;
	my ( $prevTime, $deltaT ) = ( 0, 0 );
	foreach (@$rows) {
		my $uxtime = date2unix( substr( $_, 0, 17 ) );
		if ( !$uxtime ) {
			ERROR "Could not parse date from line $_";
			next;
		}
		$deltaT = $uxtime - $prevTime;
		next if ( $deltaT == 0 ); # e.g. eph 1019,1020 one message for each sat.
		next if ( $deltaT <= 1 && $mestp =~ /10(19|20|42|43|44|45|46)|63/ );
		if ( $prevTime && $deltaT > $maxGap ) {
			$scan++;
		}
		push( @{ $scans[$scan] }, $uxtime );
		$prevTime = $uxtime;
	}

	my @repRates;
	my $highest_nof_diffs = 0;
	foreach (@scans) {
		my @timestamps = @{$_};

		# Compute the differences
		my @diffs;
		for ( my $i = 1 ; $i <= $#timestamps ; $i++ ) {
			push( @diffs, $timestamps[$i] - $timestamps[ $i - 1 ] );
		}
		my $nof_diffs = scalar @diffs;

		if ( $nof_diffs < 2 ) {
			WARN("$stat: $mestp: only $nof_diffs diffs");
			next;
		}

		my ( $mean, $prms, $median, $min, $max, $adev, $rms_n ) =
		  stats( pdl \@diffs );
		$mean  = sprintf( "%.0f",  $mean );
		$rms_n = sprintf( "%.02f", $rms_n );
		print $stat, ": ", $mestp, ": ", join( ' ', @diffs ),
		  "[Sig: $mean, $rms_n]\n";

		if ( $rms_n > 10 ) {
			WARN("$stat: $mestp: RMS too high: $rms_n");
			next;
		}

		# get the most frequent value
		my %counti = ();
		$counti{$_}++ foreach (@diffs);
		my ( $ni, $mfv ) = ( 0, 0 );
		while ( my ( $k, $v ) = each %counti ) {
			if ( $v > $ni ) {
				$ni  = $v;
				$mfv = $k;
			}
		}

		my $rounded_val = $mfv;    # init
		foreach ( ( 1, 5, 10, 15, 30, 60, 120, 150, 300 ) )
		{                          # most likely values
			my $mdiff = abs( $mfv - $_ );
			if ( $mdiff <= 2 ) {
				$rounded_val = $_;
			}
		}
		push( @repRates, [ $rounded_val, $nof_diffs ] );

		if ( $nof_diffs > $highest_nof_diffs ) {
			$highest_nof_diffs = $nof_diffs;
		}
	}    # -----  end foreach scan -----

	my @mostLikelyRates = grep { $_->[1] == $highest_nof_diffs } @repRates;
	my $mostLikelyRate  = $mostLikelyRates[0]->[0];
	foreach (@repRates) {
		if ( abs( $_->[0] - $mostLikelyRate ) > 2 ) {
			ERROR
"$stat: $caAbbr: $mestp: repetition rates from different scans differ: $mostLikelyRate $_->[0]";
			if ( scalar @repRates == 2 ) {
				return;
			}
		}
	}

	return $mostLikelyRate;
}

# =============================================================================
# parseConfig ($confFile)
# =============================================================================
# Parse the BNC config file.
#
# Param  : $confFile [required] BNC config file
# Return : Hash with configuration on success, otherwise undef
# Usage  : $bncConf = parseConf($bncConfFile);
#          $corrMount = $bncConf->{'PPP'}->{'corrMount'};
# =============================================================================
sub parseConfig {
	my ($confFile) = @_;

	-s $confFile || LOGDIE "BNC config file \"$confFile\" does not exist\n";
	TRACE "Parse BNC config file $confFile";
	open( my $INP, '<', $confFile )
	  || die "Could not open file '$confFile': $!";
	my @confLines = <$INP>;
	close($INP);

	my %conf;
	my $section;    # [General], [PPP]
	foreach (@confLines) {
		chomp;
		s/#.*//;     # entfernt Kommentare
		s/^\s*//;    # whitespace am Anfang entfernen
		s/\s+$//;    # entfernt alle whitespaces am Ende
		next unless length;
		if ( $_ =~ /\[(\S+)\]/ ) { $section = $1 }
		next if ( !$section );
		my ( $key, $val ) = split( /\s*=\s*/, $_, 2 );
		if ( !defined $val ) { $val = "" }

		if ( $key eq "mountPoints" ) {

			# Simple parsing
			$val =~ s/^\/\///;
			my @mpts = split( /,\s?\/{2}/, $val );
			$conf{$section}->{$key} = \@mpts;

			# Extended parsing
			my @mpts_def = ();
			foreach (@mpts) {

			#  user:passwd@igs-ip.net:2101/ASPA0 RTCM_3.0 ASM -14.33 189.28 no 1
				if ( $_ =~
/^([\w-]+):(.+[^@])@([\w\.-]+):(\d{3,5})\/([-\w]+) ([\w\.]+) ?(\w{3})? ([\+\-\d\.]+) ([\+\-\d\.]+) no (\w+)/i
				  )
				{
					push(
						@mpts_def,
						{
							caster    => $3,
							port      => $4,
							mp        => $5,
							ntripVers => $10
						}
					);
				}
				else {
					ERROR "$confFile: Could not parse mountPoints string $_";
				}
			}
			$conf{$section}->{'mountPoints_parsed'} = \@mpts_def;
		}
		elsif ( $key eq "cmbStreams" ) {
			my @cmbStrs = split( /\s*,\s*/, $val );
			foreach (@cmbStrs) {
				s/"//g;
				s/\s+$//;    # entfernt alle whitespaces am Ende
			}
			$conf{$section}->{$key} = \@cmbStrs;
		}
		else { $conf{$section}->{$key} = $val }
	}

	my @nofPar = keys %conf;
	if ( scalar @nofPar < 1 ) {
		ERROR "No parameter found in BNC conf \"$confFile\"";
		return;
	}
	return \%conf;
}

# =============================================================================
# parseLogfile ($file, $sampling, $goBackSecs, $logMode )
# =============================================================================
# Parse BNCs' logfile
#
# Param  : $file       [required] BNC logfile
#          $sampling   [optional] sampling rate for logfile
#          $logMode    [optional] Flag. If set, remember the position of the file-read
#                      for the next read. Default: off
# Return : \%data
# =============================================================================
sub parseLogfile {
	my $file     = shift;
	my $sampling = shift // 1;
	my $logMode  = shift // 0;

	open( my $fh, "<", $file ) || LOGDIE "Could not open file $file: $!\n";

	# Goto last position from last read
	#my $fPos = filePosition($file);
	#TRACE "Current file pos: $fPos";
	$logMode && seek( $fh, &Common::filePosition($file), 0 );

	#$logMode && seek ( $fh, $fPos, 0 );
	my $ln = "";
	my ( @hlp, @epochs, @latencies, @restarts );
	my $rec = {};
	while (<$fh>) {
		chomp( $ln = $_ );
		$rec = {};

		if ( $ln =~ /\bNEU/ ) {    # NEU displacements
			@hlp = split( /\s+/, $ln );
			my $tp = Time::Piece->strptime( substr( $hlp[2], 0, 19 ),
				'%Y-%m-%d_%H:%M:%S' );

			if (   $hlp[14] eq '-nan'
				|| $hlp[15] eq '-nan'
				|| $hlp[16] eq '-nan' )	{
				WARN("$hlp[2] $hlp[3]: NEU displacements are NAN");
				next;
			}

			#DEBUG ($tp->epoch, $hlp[3]);
			push(
				@epochs,
				{
					time => $tp->epoch,
					site => $hlp[3],
					dN   => $hlp[14],
					dE   => $hlp[15],
					dU   => $hlp[16],
					TRP  => $hlp[18] + $hlp[19],
				}
			);
		}
		elsif ( index( $ln, "latency", 25 ) > 1 ) {

# DEBUG ($ln);
# altes format:      15-10-06 15:29:02 POTS0: Mean latency 2.34 sec, min 1.58, max 3, rms 0.48, 43 epochs, 17 gaps
# neu in BNC 2.12.4: 17-06-06 15:35:02 OHI37 Observations: Mean latency 1.51 sec, min 0.57, max 2.7, rms 0.5, 203 epochs, 73 gaps
			@hlp = split( /\s+/, $ln );

			# old latency log format
			if ( $hlp[2] =~ /:$/ ) {
				splice @hlp, 3, 0, 'Placeholder:';
				$hlp[2] =~ s/:$//;
			}
			$hlp[3] =~ s/:$//;

			my $tp =
			  Time::Piece->strptime( "$hlp[0] $hlp[1]", '%y-%m-%d %H:%M:%S' );
			$rec = {
				time    => $tp->epoch,
				mp      => $hlp[2],
				meanLat => $hlp[6] + 0.0,
				epochs  => int( $hlp[14] ),
				type    => $hlp[3]
			};

		   # Unter bestimmten Bedingungen werden die gaps nicht rausgeschrieben!
			if ( $ln =~ /gaps/ ) {
				$rec->{'gaps'} = int( $hlp[16] );
			}

			push( @latencies, $rec );
		}
		elsif ( index( $ln, "Start BNC" ) > 1 ) {

			# 17-06-13 07:06:58 ========== Start BNC v2.12.3 (LINUX) ==========
			@hlp = split( /\s+/, $ln );
			my $tp =
			  Time::Piece->strptime( "$hlp[0] $hlp[1]", '%y-%m-%d %H:%M:%S' );
			push(
				@restarts,
				{
					time    => $tp->epoch,
					bncvers => $hlp[5]
				}
			);
		}

	}    # -----  next line  -----

	$logMode
	  && &Common::filePosition( $file, tell($fh) ); # Remember pos for next read
	close $fh;

	# Sampling must be done afterwords, for each station separated!
	my @epochs_sampled;
	my @sites = map { $_->{'site'} } @epochs;

	#@sites = uniq @sites;
	my %hlp1 = ();
	@sites = grep { !$hlp1{$_}++ } @sites;
	foreach my $s (@sites) {
		my $epoch_selected = 0;
		foreach my $rec (@epochs) {
			next if ( $rec->{'site'} ne $s );
			if ( $rec->{'time'} - $epoch_selected >= $sampling ) {
				push( @epochs_sampled, $rec );
				$epoch_selected = $rec->{'time'};
			}
		}
	}

	my %data = (
		EPOCHS    => \@epochs_sampled,
		LATENCIES => \@latencies,
		RESTARTS  => \@restarts
	);

	return \%data;
}

# =============================================================================
# parsePPPLogfile ($file, $sampling, $goBackSecs, $logMode )
# =============================================================================
# Parse BNCs' PPP station logfile
#
# Param  : $file       [required] BNC PPP station
#          $sampling   [optional] sampling rate for logfile
#          $goBackSecs [optional] go back that seconds from now in logfile
#          $logMode    [optional] Flag. If set, remember the position of the file-read
#                      for the next read. Default: off
# Return : $dateStr, $station, $ssrData \%data
# =============================================================================
sub parsePPPLogfile {
	my $file       = shift;
	my $sampling   = shift // 1;
	my $goBackSecs = shift // 0;
	my $logMode    = shift // 0;

	if ($logMode) { $goBackSecs = 0 }

	my $startSec;
	if ($goBackSecs) {
		$startSec = time() - $goBackSecs;
	}
	my $epo_ppp;
	my $old_epochSec = 0;
	my $epochSec     = 0;
	my $epochDiff    = 0;

    my ( @EPOCH_PPP);
	my ( @hlp, @N, @E, @U, @ISFIX, @NUMFIX, @TRP );
	my ( %SATNUM, %RECCLK, %AMB, %RES, %ELE, %ION, %BIA );
	my ( $date, $station, $lki, $sys, $sat, $amb );

	my ( @EPOCH_SSR);
	my ( %CLKCORR);
	my ( %YAW_SSR);
	my ( %YAW_DEF);
	my ( %CODEBIAS);
	my ( %PHASEBIAS);
	my ( %JUMPCOUNT);
	my $epochSsrSec = 0;
	my $ssrData;
	my $dateStr;

	open( my $fh, "<", $file ) || LOGDIE "Could not open file $file: $!\n";

	# Goto last position from last read
	#my $fPos = filePosition($file);
	#TRACE "Current file pos: $fPos";
	$logMode && seek( $fh, filePosition($file), 0 );

	#$logMode && seek ( $fh, $fPos, 0 );
	my $ln = "";
	while (<$fh>) {
		chomp( $ln = $_ );
		if ( $ln =~ /\bof Epoch\b/ ) {
			# PPP of Epoch 2015-08-27_14:00:15.000
			if ( $ln =~ /PPP of Epoch (\d{4}-\d{2}-\d{2}_\d{2}:\d{2}:\d{2})\.\d+/ ) {
				$epo_ppp = $1;    #print "$epo_ppp\n";
				@hlp = split( /\s+/, $ln );
                $ssrData = $hlp[5]; #print "$ssrData\n";
                $dateStr = substr( $hlp[3], 0, 10 );

			}
			else {
				ERROR "strange ppp line: \"$ln\""; next
			}

			my $tp = Time::Piece->strptime( $epo_ppp, '%Y-%m-%d_%H:%M:%S' );
			$epochSec  = $tp->epoch();
			$epochDiff = $epochSec - $old_epochSec;
			next;
		}

		next if ( !$epo_ppp );
		next if ( defined $startSec && $epochSec < $startSec );
		next if ( $epochDiff && $epochDiff < $sampling );

		@hlp = split( /\s+/, $ln );

		if ( $ln =~ /\bdN\b/ ) {
			#2025-11-29_16:55:09.000 WTZR00DEU0 X = 4075580.2482 +- 0.0500 Y = 931854.1736 +- 0.0314 Z = 4801568.3044 +- 0.0591 dN = 0.0074 +- 0.0407 dE = 0.0895 +- 0.0284 dU = -0.0072 +- 0.0672 fix 96 %
			push( @EPOCH_PPP, $epochSec );
			$old_epochSec = $epochSec;
			$date = substr( $hlp[0], 0, 9 );
			$station = $hlp[1];
		    my $numFix = 0;
		    my $isFix;
		    my $strFix = "fix";
			if (   $hlp[19] eq '-nan'
				|| $hlp[24] eq '-nan'
				|| $hlp[29] eq '-nan' )	{
				WARN("$hlp[0] $station: NEU displacements are NAN");
			}
			push @N, $hlp[19];
			push @E, $hlp[24];
			push @U, $hlp[29];

			if ( $hlp[32] eq $strFix ) {
				$isFix  = 1;
				$numFix = int( $hlp[33] );
			}
			else {
				$isFix  = 0;
				$numFix = 0;
			}
			push @ISFIX,  $isFix;
			push @NUMFIX, $numFix;
		}
		elsif ( ( $ln =~ /\bAMB\b/ ) && ( $ln !~ /RESET/ ) ) {		 # 2015-08... AMB lIF G04 253.0000 -8.9924 +- 1.7825 el = 22.03 epo = 86
			$lki = $hlp[2];
			$sat = $hlp[3];
			$sys = substr( $sat, 0, 1 );
			$amb = $hlp[4] + $hlp[5];
			push @{ $AMB{$lki}{$sys}{$sat}{EPOCH_PPP} },  $epochSec;
			push @{ $AMB{$lki}{$sys}{$sat}{DATA} },   $amb;
			push @{ $AMB{$lki}{$sys}{$sat}{NUMEPO} }, $hlp[13];
			push @{ $ELE{$sys}{$sat}{EPOCH_PPP} }, $epochSec;
			push @{ $ELE{$sys}{$sat}{DATA} },  $hlp[10];
		}
		elsif ( $ln =~ /\bRES\b/ && $ln !~ /Neglected/ ) {			# 2015-08... RES lIF G30  -0.0076
			$sat = $hlp[3];
			$lki = $hlp[2];
			$sys = substr( $sat, 0, 1 );

			#print "$epo $lki $sys $sat $res\n";
			push @{ $RES{$lki}{$sys}{$sat}{EPOCH_PPP} }, $epochSec;
			push @{ $RES{$lki}{$sys}{$sat}{DATA} },  $hlp[4];
		}
		elsif ( ( $ln =~ /\bION\b/ ) && ( $ln !~ /RESET/ ) ) {		# 2018-12-01_20:37:58.000 ION      G02     0.0000    -0.3277 +-   2.4663
			$sat = $hlp[2];
			$sys = substr( $sat, 0, 1 );
			push @{ $ION{$sys}{$sat}{EPOCH_PPP} }, $epochSec;
			push @{ $ION{$sys}{$sat}{DATA} },  $hlp[4];
		}
		elsif ( ( $ln =~ /\bBIA\b/ ) && ( $ln !~ /RESET/ ) ) {		# 2020-12-09_00:55:19.000 BIA  c1  G       0.0000    +2.5149 +-   9.6543
			$lki = $hlp[2];
			$sys = $hlp[3];
			push @{ $BIA{$lki}{$sys}{EPOCH_PPP} }, $epochSec;
			push @{ $BIA{$lki}{$sys}{DATA} },  $hlp[4] + $hlp[5];
		}
		elsif ( $ln =~ /\bSATNUM\b/ ) {    # 2015-09... SATNUM G  8
			push( @{ $SATNUM{ $hlp[2] } }, $hlp[3] );
		}
		elsif ( $ln =~ /\bTRP\b/ ) {  # 2015-08... TRP  2.3803 +0.1009 +- 0.0324
			push( @TRP, $hlp[2] + $hlp[3] );
		}
		elsif ( $ln =~ /\bREC_CLK\b/ ) {		 # 2024-10-20_03:57:30.000 RECCLK  G       0.0000    -1.7861 +-   0.5268
			$sys = $hlp[2];
			push @{ $RECCLK{$sys}{EPOCH_PPP} }, $epochSec;
			push @{ $RECCLK{$sys}{DATA} },  $hlp[3] + $hlp[4];
		}
		##################
		# SSR parameters #
		##################
	    elsif ( $ln =~ /\bclkCorr\b/ && @hlp > 3) {
	    	# clkCorr 2025-11-17_13:50:45.000 C45 -18.576
	    	if ( $ln =~ /clkCorr (\d{4}-\d{2}-\d{2}_\d{2}:\d{2}:\d{2})\.\d+/ ) {
				$epochSsrSec = $1;    #print "$epochSsrSec\n";
			}
			else {
				ERROR "strange ssr line: \"$ln\""; next
			}
			my $tp = Time::Piece->strptime( $epo_ppp, '%Y-%m-%d_%H:%M:%S' );
			$epochSsrSec  = $tp->epoch();
			$date = substr( $hlp[0], 0, 9 );
			$sat = $hlp[2];
			$sys = substr( $sat, 0, 1 );
			push @{ $CLKCORR{$sys}{$sat}{EPOCH_SSR} }, $epochSsrSec;
			push @{ $CLKCORR{$sys}{$sat}{DATA} },  $hlp[3];
		}
	    elsif ( $ln =~ /\byawAngle\b/&& @hlp > 3 ) {
	    	# yawAngle 2025-11-17_13:50:40.000 G06 218.672 136.115
	    	if ( $ln =~ /yawAngle (\d{4}-\d{2}-\d{2}_\d{2}:\d{2}:\d{2})\.\d+/ ) {
				$epochSsrSec = $1;    #print "$epochSsrSec\n";
			}
			else {
				ERROR "strange ssr line: \"$ln\""; next
			}
			my $tp = Time::Piece->strptime( $epo_ppp, '%Y-%m-%d_%H:%M:%S' );
			$epochSsrSec  = $tp->epoch();
			$sat = $hlp[2];
			$sys = substr( $sat, 0, 1 );
			push @{ $YAW_SSR{$sat}{EPOCH_SSR} }, $epochSsrSec;
			push @{ $YAW_SSR{$sat}{DATA} },  $hlp[3];
			push @{ $YAW_DEF{$sat}{EPOCH_SSR} }, $epochSsrSec;
			push @{ $YAW_DEF{$sat}{DATA} },  $hlp[4];
		}
	    elsif ( $ln =~ /\bcodeBias\b/ && @hlp > 3) {
	    	#codeBias 2025-11-17_13:50:35.000 C45  1P  12.140  2I  13.330  5P  21.740  6I  20.190
	    	if ( $ln =~ /codeBias (\d{4}-\d{2}-\d{2}_\d{2}:\d{2}:\d{2})\.\d+/ ) {
				$epochSsrSec = $1;    #print "$epochSsrSec\n";
			}
			else {
				ERROR "strange ssr line: \"$ln\""; next
			}
			my $tp = Time::Piece->strptime( $epo_ppp, '%Y-%m-%d_%H:%M:%S' );
			my ($num_bias, $bias_type, $bias_value);
			$epochSsrSec  = $tp->epoch();
			$sat = $hlp[2];
			$sys = substr( $sat, 0, 1 );
			my $hlpSize = @hlp;
			for (my $i=3; $i < $hlpSize; $i++) {
              $bias_type  = $hlp[$i++];
              $bias_value = $hlp[$i];
              push @{ $CODEBIAS{$bias_type}{$sys}{$sat}{EPOCH_SSR} }, $epochSsrSec;
			  push @{ $CODEBIAS{$bias_type}{$sys}{$sat}{DATA} },  $bias_value;
            }
		}
	    elsif ( $ln =~ /\bphaseBias\b/ && @hlp > 3) {
	    	#phaseBias 2025-11-17_13:50:55.000 G02  315.00   0.000  1C  1 x  0.291  2W  1 x  0.408
	    	if ( $ln =~ /phaseBias (\d{4}-\d{2}-\d{2}_\d{2}:\d{2}:\d{2})\.\d+/ ) {
				$epochSsrSec = $1;    #print "$epochSsrSec\n";
			}
			else {
				ERROR "strange ssr line: \"$ln\""; next
			}
			my $tp = Time::Piece->strptime( $epo_ppp, '%Y-%m-%d_%H:%M:%S' );
			my ($num_bias, $bias_type, $bias_value, $jump_count, $int_flag);
			$epochSsrSec  = $tp->epoch();
			$sat = $hlp[2];
			$sys = substr( $sat, 0, 1 );
			my $hlpSize = @hlp;
			for (my $i=5; $i < $hlpSize; $i++) {
              $bias_type  = $hlp[$i++]; #print "$sat type: $bias_type";
              $jump_count = $hlp[$i++]; #print " count: $jump_count";
              $int_flag   = $hlp[$i++]; #print " flag: $int_flag";
              $bias_value = $hlp[$i];	#print " bias: $bias_value \n";
              push @{ $PHASEBIAS{$bias_type}{$sys}{$sat}{EPOCH_SSR} }, $epochSsrSec;
              push @{ $JUMPCOUNT{$bias_type}{$sys}{$sat}{EPOCH_SSR} }, $epochSsrSec;
			  push @{ $PHASEBIAS{$bias_type}{$sys}{$sat}{DATA} },  $bias_value;
			  push @{ $JUMPCOUNT{$bias_type}{$sys}{$sat}{DATA} },  $jump_count;
            }
		}
	}    # -----  next line  -----

	$logMode && filePosition( $file, tell($fh) );   # Remember pos for next read
	close $fh;

	my $nof_epochs = scalar @EPOCH_PPP;
	DEBUG(  "$station: epochs:$nof_epochs, North displac.: "
		  . scalar @N
		  . ", East displac.: "
		  . scalar @E
		  . ", Up displac.: "
		  . scalar @U
		  . ", isfix: "
		  . scalar @ISFIX
		  . ", numfix: "
		  . scalar @NUMFIX
		  . ", TRP:"
		  . scalar @TRP );
	if ( $nof_epochs != scalar @N ) {
		LOGDIE "number of epochs and residuals not equal\n";
	}
	if ( $nof_epochs != scalar @TRP ) {
		LOGDIE "number of epochs and TRP not equal\n";
	}

	if ( !$station ) { WARN "could not grep stationname from file: $file\n" }

	my %data = (
		EPOCH_PPP  => \@EPOCH_PPP,
		N          => \@N,
		E          => \@E,
		U          => \@U,
		ISFIX      => \@ISFIX,
		NUMFIX     => \@NUMFIX,
		TRP        => \@TRP,
		SATNUM     => \%SATNUM,
		RECCLK     => \%RECCLK,
		RES        => \%RES,
		AMB        => \%AMB,
		ELE        => \%ELE,
		ION        => \%ION,
		BIA        => \%BIA,
		EPOCH_SSR  => \@EPOCH_SSR,
		CLKCORR    => \%CLKCORR,
		YAW_SSR    => \%YAW_SSR,
		YAW_DEF    => \%YAW_DEF,
		CODEBIAS   => \%CODEBIAS,
		PHASEBIAS  => \%PHASEBIAS,
		JUMPCOUNT  => \%JUMPCOUNT,
	);

	return ($dateStr, $station, $ssrData, \%data, 0 );
}

# =============================================================================
# BncStillWorks ($bncConfFile)
# =============================================================================
# Checks if BNC is still working.
#
# BNC Jobs can still be alive (in processlist) but are not producing any more.
# This function checks if a BNC process is proper working.
#
# Param  : $bncConfFile [required] path of BNC config file
# Return : true if BNC is still working otherwise false.
# =============================================================================
sub BncStillWorks {
	my ($bncConfFile) = @_;

	my $timep = Time::Piece->new;

	# for safety if it is exatly at 00:00, add 30 sec
	my $min_tmp = $timep->strftime("%M");
	if ( $min_tmp =~ /00|15|30|45/ && $timep->strftime("%S") < 15 ) {
		$timep += 30;
		sleep 30;
	}
	my $yyyy = $timep->year;
	my $yy   = $timep->yy;
	my $doy  = sprintf "%03d", $timep->yday + 1;
	my $hh   = $timep->strftime("%H");
	my $h    = uc( chr( 65 + $hh ) );
	my $min  = $timep->min;
	my $startmin;
	if    ( $min < 15 )  { $startmin = "00" }
	elsif ( $min < 30 )  { $startmin = "15" }
	elsif ( $min < 45 )  { $startmin = "30" }
	elsif ( $min <= 59 ) { $startmin = "45" }
	my $bncConf        = parseConf($bncConfFile);
	my $bncLogFileStub = $bncConf->{'General'}->{'logFile'};

	# BNC log file
	# ------------
	my $bncLogFile =
	  "${bncLogFileStub}_" . $timep->strftime("%y%m%d");    # -> bnc.log_160425
	unless ( -s $bncLogFile ) {
		WARN("BNC logfile \"$bncLogFile\" is empty or does not exist");
		return 0;
	}

	# RINEX Obs Generation
	# --------------------
	if ( $bncConf->{'General'}->{'rnxPath'} ) {
		my $rnxPath = $bncConf->{'General'}->{'rnxPath'};
		$rnxPath =~ s/\/$//;

# Write Rnx3 files (i.e. long Rnx3 filenames) 2: on ('rnxV3filenames' is deprecated since 2.12.8!!!)
		my $writeRnxV3 = $bncConf->{'General'}->{'rnxV3'};
		my $rnxIntr    = $bncConf->{'General'}->{'rnxIntr'};
		my $fileMask;

		if ($writeRnxV3) {
			if ( $rnxIntr eq "1 hour" ) {
				$fileMask = "*_S_${yyyy}${doy}${hh}??_01H_30S_?O.rnx";
			}
			elsif ( $rnxIntr eq "15 min" ) {
				$fileMask = "*_S_${yyyy}${doy}${hh}${startmin}_15M_01S_?O.rnx";
			}
			else {    # daily?
				$fileMask = "*_S_${yyyy}${doy}????_01D_30S_?O.rnx"
				  ;    # HRAG00ZAF_S_20191220000_01D_30S_MO.rnx
			}
		}
		else {         # Rnx2
			if ( $rnxIntr eq "1 hour" ) {
				$fileMask = "????${doy}${h}.${yy}O";
			}
			elsif ( $rnxIntr eq "15 min" ) {
				$fileMask = "????${doy}${h}${startmin}.${yy}O";
			}
			else {     # daily?
				$fileMask = "????${doy}*.${yy}O";
			}
		}

		my @rnxFiles = glob "$rnxPath/$fileMask";
		if ( scalar @rnxFiles < 1 ) {
			ERROR(
"BNC does not create RINEX Obs files. (Filemask: \"$fileMask\" Path: $rnxPath)"
			);

			#return 0;
		}
	}

	# RINEX Ephemerides Generation
	# ----------------------------
	if ( $bncConf->{'General'}->{'ephPath'} ) {
		my $rnxPath = $bncConf->{'General'}->{'ephPath'};
		$rnxPath =~ s/\/$//;
		my $writeRnxV3 = $bncConf->{'General'}->{'ephV3'};
		my $rnxIntr    = $bncConf->{'General'}->{'ephIntr'};
		my $fileMask;

		if ($writeRnxV3) {
			if ( $rnxIntr eq "1 hour" ) {
				$fileMask = "BRD?00WRD_S_${yyyy}${doy}${hh}00_01H_?N.rnx";
			}
			elsif ( $rnxIntr eq "15 min" ) {
				$fileMask =
				  "BRD?00WRD_S_${yyyy}${doy}${hh}${startmin}_15M_?N.rnx"
				  ;    # BRDC00WRD_S_20191220900_15M_MN.rnx
			}
			else {     # daily?
				$fileMask = $fileMask =
				  "BRD?00WRD_S_${yyyy}${doy}0000_01D_?N.rnx";
			}
		}
		else {         # Rnx2
			$fileMask = "BRD?${doy}*.${yy}N";
		}

		my @rnxFiles = glob "$rnxPath/$fileMask";
		if ( scalar @rnxFiles < 1 ) {
			ERROR(
"BNC does not create RINEX Nav files. (Filemask: \"$fileMask\" Path: $rnxPath)"
			);

			#return 0;
		}
	}

	# Check jobs making PPP
	# ---------------------
	if ( $bncConf->{'PPP'}->{'corrMount'} && $bncConf->{'PPP'}->{'staTable'} ) {
		my $timeOfLastCoo =
		  `grep "NEU:" $bncLogFile | tail -1 | cut -d ' ' -f1,2`;
		chomp $timeOfLastCoo;
		if ( !$timeOfLastCoo ) {
			ERROR "BNC does not compute coordinates";
			return 0;
		}

		my $tp  = Time::Piece->strptime( $timeOfLastCoo, '%y-%m-%d %H:%M:%S' );
		my $now = Time::Piece->new;
		my $tdiff = $now - $tp;
		if ( $tdiff > 1200 ) {
			ERROR(
				"Last computed coordinates are " . $tdiff / 60 . " min old" );
			return 0;
		}
	}

	# BNC works
	return 1;
}

1;    # End of Bnc
