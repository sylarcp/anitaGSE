#!/usr/bin/perl

# Daemon which monitors directory with new packet links and starts data 
# processing program

# $Header$



require 'AnitaConfig.pi';

use strict;
use vars qw($opt_h $opt_d $opt_t $opt_D $myPIDFile $log_dir $link_dir $pid_dir $exec_dir $telem_dir $dbname $losFIFO $hirateFIFO $lorateFIFO $logFile $logN $losFlag $hirateFlag $lorateFlag);
use Getopt::Std;
use File::Path;
use File::Temp;

# Usage function
sub usage{
  print "unpackd.pl [-d <val>] [-t] [-D] [-h] \n";
  print "       -d <val>  Directory to monitor for incoming packets\n";
  print "       -t        Unpack packets into files\n";
  print "       -D        Unpack data into database\n";
  print "       -h        This help\n";
  exit;
}

# Signal handling functions
sub ExitCleanly {
  my $signame=shift;
  system "killall -15 lt-unpackd";
  system "rm -f $losFIFO $hirateFIFO $lorateFIFO";
  system "rm -f $myPIDFile";
  print LOG "$$: Caught signal $signame. Bye bye!\n";
  close LOG;
  exit;
}

sub RestartDaemons {
  system "killall -15 lt-unpackd";
  print LOG "$$: Terminated daemons!\n";
  ($logFile,$logN)=logRotate($logFile,$logN);
  StartDaemons();
}

sub StartDaemons {
  my $losLog = $logFile;
  my $hirateLog = $logFile;
  my $lorateLog = $logFile;
  $losLog =~ s/\.log$/_los.log/;
  $hirateLog =~ s/\.log$/_hirate.log/;
  $lorateLog =~ s/\.log$/_lorate.log/;

  system "$exec_dir/unpack/unpackd -f $exec_dir/los.fmt $losFlag $losFIFO >> $losLog 2>&1 &";
  system "$exec_dir/unpack/unpackd -f $exec_dir/sip_hr.fmt $hirateFlag $hirateFIFO >> $hirateLog 2>&1 &";
  system "$exec_dir/unpack/unpackd -f $exec_dir/sip_lr.fmt $lorateFlag $lorateFIFO >> $lorateLog 2>&1 &";
  print LOG "$$: Started daemons!\n";
}

# Function killing previous daemon if still running and storing PID
sub StorePID{
  my $file = shift;
  if (-e $file){
    open IN,$file;
    my $pid=<IN>;
    chomp $pid;
    system "kill -15 $pid";
    close IN;
  }
  open OUT,">$file";
  print OUT "$$\n";
  close OUT;
}

sub logRotate {
  my ($file,$n) = @_;

#  my $logL=`wc -l $file`;
#  chomp $logL;
#  if($logL>10000){
    close LOG;
    $n++;
    $file = sprintf "$log_dir/unpackd.%05d.log",$n;
    open LOG,">$file";
#  }
  return ($file,$n);
}

# Get command line options
getopts('htDd:');
usage() if $opt_h;
$link_dir=$opt_d if $opt_d;
my $dotelem = $opt_t;
my $dodb = $opt_D;

# Install signal catcher
$SIG{INT} = \&ExitCleanly;
$SIG{TERM} = \&ExitCleanly;
$SIG{HUP} = \&RestartDaemons;

# Open a log file
my $logN;
opendir LOGDIR,$log_dir;
my @logFiles = grep /^unpackd.+\d+\.log$/,readdir LOGDIR;
my @sortedLogs = sort {$a cmp $b} @logFiles;
if ($#sortedLogs>=0){
  $logN=$sortedLogs[-1];
  $logN=~s/^unpackd\.//;
  $logN=~ s/\.log$//;
  $logN=int($logN)+1;
}else{
  $logN=1;
}
$logFile = sprintf "$log_dir/unpackd.%05d.log",$logN;
open LOG,">$logFile";

# Kill any other running unpackd.pl and store our PID
$myPIDFile = "$pid_dir/unpackd.pid";
StorePID($myPIDFile);

# Create our own pipes
$losFIFO = mktemp('/tmp/unpackd_los_fifo_XXXXX');
$hirateFIFO = mktemp('/tmp/unpackd_hirate_fifo_XXXXX');
$lorateFIFO = mktemp('/tmp/unpackd_lorate_fifo_XXXXX');
system "mknod $losFIFO p";
system "mknod $hirateFIFO p";
system "mknod $lorateFIFO p";

# Prepare unpackd run flags 
$losFlag="-m/tmp/anita_monitor_los.txt ";
$hirateFlag="-m/tmp/anita_monitor_hirate.txt ";
$lorateFlag="-m/tmp/anita_monitor_lorate.txt ";
if($dotelem){
    mkpath("$telem_dir/los",0,0755) unless (-e "$telem_dir/los");
    mkpath("$telem_dir/fast_tdrss",0,0755) unless (-e "$telem_dir/fast_tdrss");
    mkpath("$telem_dir/slow_tdrss",0,0755) unless (-e "$telem_dir/slow_tdrss");
    $losFlag.="-t $telem_dir/los ";
    $hirateFlag.="-t $telem_dir/fast_tdrss ";
    $lorateFlag.="-t $telem_dir/slow_tdrss ";
}
if($dodb){
    $losFlag.="-d$dbname";
    $hirateFlag.="-d$dbname";
    $lorateFlag.="-d$dbname";
}

# Start unpackd daemons
StartDaemons();

# Since this is a daemon, we put it in a top most infinite loop
while(1){
  # Open directory to monitor
  # Directory entries are actually links which are placed by data xfer
  # daemon
  my @links;
  if(-e $link_dir){
    opendir DIR,$link_dir;
    @links = grep /_\d+$/,readdir DIR;
    closedir DIR;
  }
  for my $l (@links){
    my $now=time();
    print LOG "$$: Processing $l @ $now\n";
    # Determine format to use based on file name
    my $fmt;
    if($l=~/los/){
      system "cat $link_dir/$l > $losFIFO";
    }elsif($l=~/fast_tdrss/){
      system "cat $link_dir/$l > $hirateFIFO";
    }elsif($l=~/slow_tdrss/){
      system "cat $link_dir/$l > $lorateFIFO";
    }elsif($l=~/iridium/){
      system "cat $link_dir/$l > $lorateFIFO";
    }else{
      print LOG "$$: Unable to determine format of $l. Skipping!\n";
    }
    # Remove link after processing
    system "rm -f $link_dir/$l";
  }
  # Check if log file needs to be changed
  #($logFile,$logN)=logRotate($logFile,$logN);
  # Wait a bit and then loop again
  sleep 1;
}
