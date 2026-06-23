package Common;

#
# Some common functions
#
# Revision: $Header: https://software.rtcm-ntrip.org/svn/tags/BNC_2.13.6/scripts/Common.pm 10513 2024-06-21 08:31:41Z wiese $
#

use strict;
use warnings;
use Exporter;
use Data::Dumper;
use Log::Log4perl qw(:easy);
use File::Temp;
use File::Basename;
use File::Spec::Functions;
use Fcntl ':flock';
use Proc::ProcessTable;

#$| = 1; # Buffer off (print)

sub trim {
    my ($text) = @_;
    $text =~ s/^[\s]*//g;
    $text =~ s/[\s]*$//g;
    return $text;
}

# run a command in the shell.
sub runCmd {
    my ($cmd) = shift;

    DEBUG("run cmd: \"$cmd\"");

    my $tmpfh   = File::Temp->new( UNLINK => 1, SUFFIX => '.dat' );
    my $tmpfile = $tmpfh->filename();

    $cmd = "$cmd 2>$tmpfile |";
    my $PIPE;
    unless ( open $PIPE, $cmd ) {
        ERROR("open $cmd | failed ($!)");
        return ( -1, "", "" );
    }
    my $stdout = join '', <$PIPE>;
    close $PIPE;
    my $exit_code = $?;

    my $stderr = getContent($tmpfile);
    return ( $exit_code, $stdout, $stderr );
}

sub mkDir {
    my $dir = shift;
    return 1 if ( -d $dir );

    eval { mkpath($dir) };
    if ($@) {
        warn ("Could not create path [$dir]: $@");
        return 0;
    }
    return 1;
}

#** @function amInteractiv ()
# @brief  Checks if a program runs interactivly, or e.g. was invoked by cron.
# @see    Perl Cookbook Kap. 15.2
# @return true if interactive, else false.
#*
sub amInteractiv {
    use POSIX qw(getpgrp tcgetpgrp);
    my $tty;
    open ( $tty, "<", "/dev/tty" ) || LOGDIE "Could not open /dev/tty: $!";
    my $tpgrp = tcgetpgrp( fileno ($tty) );    # get terminal foreground process group
    my $pgrp  = getpgrp ();                    # get the current process group
    close $tty;
    return ( $tpgrp == $pgrp );
}

#** @function printData ($data)
# @brief  Prints perl data structures. Uses Data::Dumper.
# @param  $data [required] Daten
# @return -
#*
sub printData {
    my $data = shift;
    my $d    = Data::Dumper->new($data);
    $d->Indent(3);    # pretty print with array indices
    print $d->Dump;
}

sub getContent {
    my ($fileName) = @_;
    my $errmsg = "";
    if ( -f $fileName ) {
        my $readMode = $/;
        undef $/;
        open ( FILE, "<", $fileName )
          or ERROR("Kann Datei nicht zum lesen oeffnen");
        $errmsg = <FILE>;
        close (FILE);
        $/ = $readMode;
    }
    return $errmsg;
}

# blurt opens a new file, prints the data in $data to it and closes the file.
# If $options->{append} is set to a true value, data will be appended to the file. Default is false, existing files will be overwritten.
#
# If the string is a Unicode string, use the utf8 option: 'blurt( $unicode_string, $file, {utf8 => 1} );'
sub blurt {
    my ( $data, $file, $options ) = @_;

    # legacy signature
    if ( defined $options and ref $options eq "" ) {
        $options = { append => 1 };
    }

    $options = {} unless defined $options;

    local $Log::Log4perl::caller_depth = $Log::Log4perl::caller_depth + 1;

    $options->{append} = 0 unless defined $options->{append};

   #_confirm( ( $options->{append} ? "Appending" : "Writing" ) . " " . length ($data) . " bytes to $file" ) or return 1;

    open my $fh, ">" . ( $options->{append} ? ">" : "" ) . $file
      or LOGCROAK("Cannot open $file for writing ($!)");

    binmode $fh;    # Win32 wants that

    if ( $options->{utf8} ) {
        binmode $fh, ":utf8";
    }

    print $fh $data
      or LOGCROAK("Cannot write to $file ($!)");
    close $fh
      or LOGCROAK("Cannot close $file ($!)");
}

#** @function filePosition ($file, $filePos)
# @brief Save position of a file in an idxfile.
#
# Especially for logfiles. For performance reasons, logfiles should not parsed always from the
# beginning, the next read access should start at the pos of the last read.
# Saves the position of the last read in a help-file ".filePos" in
# the directory of the logfile. If the param $filePos is set, then this position will be saved (Write).
# Otherwise the current position will be returned (Read).
# @param  $file    [required] File of interest
# @param  $filePos [optional] if given, this position ()in Bytes) will be saved, else return current position
# @return file position in Bytes from beginning of the file
#*
sub filePosition {
    my ( $file, $filePos ) = @_;

    my ( $filName, $filDir ) = fileparse($file);
    my $idxFil = catfile( $filDir, ".filePos" );

    if ($filePos) {

        # Save this position in idxfile
        my $newPos = "$filName $filePos";
        if ( -e $idxFil ) {
            unless ( replace( $idxFil, "^${filName} \\d+", $newPos ) ) {

                # could not replace -> append
                blurt( "$newPos\n", $idxFil, { append => 1 } );

                #                my $INP;
                #                if ( !open ( $INP, ">>", $idxFil ) ) {
                #                    ERROR("Could not append file [$idxFil]: $!");
                #                    return 0;
                #                }
                #                print $INP "$newPos\n";
                #                close ($INP);
            }
        }
        else {
            # initial write
            blurt( "$newPos\n", $idxFil );

            #writeTextInFile( $idxFil, "$newPos\n" );
        }
    }
    else {

        # Read current position from idxfile
        # ----------------------------------
        if ( !-e $idxFil ) {
            return 0;
        }
        my $line = "";
        my $idx_fh;
        if ( !open ( $idx_fh, "<", $idxFil ) ) {
            ERROR("Could not open for read $idxFil: $!");
            return 0;
        }
        while (<$idx_fh>) {
            if ( $_ =~ /^$filName (\d+)/ ) {    # RegEx problematic with getContentFiltered()
                $line .= $_;
                $filePos = $1;
                last;
            }
        }
        close ($idx_fh);

        if ( $line && !$filePos ) {
            ERROR("Could not read file position in idxfile $idxFil. Weired line \"$line\"");
            $filePos = 0;
        }

        if ($filePos) {

            # If the file was meanwhile newly created, e.g. after restart from BNC
            my $fileSize = -s $file;
            if ( $filePos > $fileSize ) {
                WARN "file $file seams to be newly created";
                DEBUG("size:$fileSize pos:$filePos");
                $filePos = 0;
            }
        }
        else {
            WARN("No position entry found in idxfile for \"$filName\"");
            $filePos = 0;
        }
    }

    return $filePos;
}

#** @function replace ($fileName, $pattern, $replacement)
# @brief  Search and replace a string in a file
# @param  $fileName    [required] file with full path
# @param  $pattern     [required] Search pattern string
# @param  $replacement [required] Replacement string
# @return true on success, false on error or patter not found
#*
sub replace {
    my ( $fileName, $pattern, $replacement ) = @_;

    # TODO getOptions();

    TRACE "Search and replace string \'$pattern\' in file $fileName";

    my $fileName_tmp = "${fileName}.tmp";
    my ( $OLD, $NEW );
    if ( !open ( $OLD, "<", $fileName ) ) {
        ERROR("Could not open file [$fileName]: $!");
        return 0;
    }
    flock ( $OLD, LOCK_SH ) || ERROR "Could not lock file '$fileName'";    # lock shared (others can read)

    if ( !open ( $NEW, ">", $fileName_tmp ) ) {
        ERROR("Could not write file [$fileName_tmp]: $!");
        flock ( $OLD, LOCK_UN ) || ERROR "Could not unlock file '$fileName'";
        close ($OLD)            || ERROR "Could not close file $fileName: $!";
        return 0;
    }

    my $numOfReplacements = 0;
    while (<$OLD>) {
        if ( $_ =~ /$pattern/ ) {
            $_ =~ s/$pattern/$replacement/;
            $numOfReplacements++;
        }
        print $NEW $_;
    }
    flock ( $OLD, LOCK_UN ) || ERROR "Could not unlock file '$fileName'";
    close ($OLD) || ( ERROR "Could not close file $fileName: $!"     && return 0 );
    close ($NEW) || ( ERROR "Could not close file $fileName_tmp: $!" && return 0 );

    unless ($numOfReplacements) {
        WARN "Pattern not found in file";
        unlink ("$fileName_tmp");
        return 0;    # ist das ein Fehler?
    }
    else {
        TRACE "$numOfReplacements pattern replaced";
        rename ( $fileName,     "${fileName}.orig" ) || ( ERROR "Could not rename file $fileName: $!" && return 0 );
        rename ( $fileName_tmp, $fileName )
          || ( ERROR "Could not rename file $fileName_tmp to $fileName: $!" && return 0 );
        unlink ("${fileName}.orig") || ERROR "Could not remove file ${fileName}.orig";
        return 1;
    }

}

#** @function isAlreadyRunning ()
# @brief   Check if this program is already running.
# @details Usage: isAlreadyRunning() && die "Same job is already running!\n";@n
#          Purpose: e.g. to avoid a program running twice
# @see     bernese gpsutil::check_job()
# @return  pid of already running job else false
#*
sub isAlreadyRunning {
    my ($scriptName) = fileparse($0);

    use Proc::ProcessTable;
    my $pt        = new Proc::ProcessTable;
    my $pid_found = 0;                        # from already running job
    foreach my $p ( @{ $pt->table } ) {
        my $cmd = $p->cmndline;
        if ( $cmd =~ /$scriptName/ && $p->pid != $$ ) {
            next if ( $p->pid == getppid () );    # when started from cron!
            DEBUG( "Found: " . $p->pid . ", myself: $$" );
            $pid_found = $p->pid;
            DEBUG "Process $cmd is running";
            last;
        }
    }

    return $pid_found;
}

# toLineProtocol($rcd)
# @brief  converts $rcd to InfluxDB line protocol
# @see    https://docs.influxdata.com/influxdb/v2.1/reference/syntax/line-protocol/
sub toLineProtocol {
    my ($rcd) = @_;

    my $err = 0;
    my $str = $rcd->{'measurement'} . ',';

    # Tags (sort by key (in lexicographic order) for performance reasons)
    foreach ( sort { lc ($a) cmp lc ($b) } keys %{ $rcd->{'tags'} } ) {
        if ( !exists $rcd->{'tags'}->{$_} ) {
            $err++;

            #next; # ?
        }
        $str .= $_ . "=" . $rcd->{'tags'}->{$_} . ",";
    }
    chop $str;
    $str .= " ";

    # Fields
    foreach ( sort { lc ($a) cmp lc ($b) } keys %{ $rcd->{'fields'} } ) {
        if ( !exists $rcd->{'fields'}->{$_} ) {
            $err++;

            #next; # ?
        }
        $str .= $_ . "=" . $rcd->{'fields'}->{$_} . ",";
    }
    chop $str;
    $str .= " " . $rcd->{'time'};    # prec=s
    if ($err) {
        return 0;
    }
    return $str;
}

#** @function rms ($numbers)
# @brief  Compute the Root mean square.
# @param  $numbers [required] List with your numbers
# @return RMS value
# @note   use PDL stats() if installed!
# @see    http://rosettacode.org/wiki/Averages/Root_mean_square#Perl
#*
sub rms {
    my $r = 0;
    $r += $_**2 for @_;
    return sqrt ( $r / @_ );
}

#** @function avg ($numbers)
# @brief  Compute the arithmetic mean value.
# @param  $numbers [required] List with your numbers
# @note   use PDL stats() if installed!
# @return mean value
# @see    http://rosettacode.org/wiki/Average/Arithmetic_mean#Perl
#*
sub avg {
    @_ or return 0;
    my $sum = 0;
    $sum += $_ foreach @_;
    return $sum / @_;
}

#** @function sigma ($numbers)
# @brief  Compute the STDDEV of the arithmetic mean.
# @param  $numbers [required] List with your numbers
# @return Sigma value
# @note   use use PDL::Primitive qw(stats) if installed!
#*
sub sigma {
    my $n = scalar (@_);

    if ( $n == 1 ) {
        WARN "Could not compute sigma for only one sample";
        return;
    }

    my $e = 0;    # arithm. Mittel
    $e += $_ for @_;
    $e = $e / $n;

    my @v  = map { $e - $_ } @_;
    my $vv = 0;
    $vv += $_**2 for @v;
    return sqrt ( $vv / ( $n * ( $n - 1 ) ) );
}

#** @function median (@numbers)
# @brief  Compute the median of a list of numbers.
# @param  @numbers [required] List with your numbers
# @see    https://en.wikipedia.org/wiki/Median
# @note   use PDL stats() if installed!
# @return median
#*
sub median {
    my $mid    = int ( @_ / 2 );
    my @sorted = sort { $a <=> $b } (@_);
    if ( @sorted % 2 ) {
        return $sorted[$mid];
    }
    else {
        return ( $sorted[ $mid - 1 ] + $sorted[$mid] ) / 2;
    }

    return;    # undef
}

1;             # End of Common

