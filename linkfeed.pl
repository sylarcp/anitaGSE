#!/usr/bin/perl

# Daemon which monitors directory with new packet links and sends them to appropriate
# processing program through a predefined pipe

# $Header$

use strict;

my $match=$ARGV[0];
die "$$: Must give match argument and link directory to match link." unless $match;
my $link_dir='.';
$link_dir=$ARGV[1] if $ARGV[1];

# These are predefined data FIFOs
my $FIFO;
if(!($match cmp "los_odd")){
  $FIFO="/tmp/unpackd_fifo_los_odd";
  $match=~s/odd/\\d+_\\d+[13579]\$/;
}elsif(!($match cmp "los_even")){
  $FIFO="/tmp/unpackd_fifo_los_even";
  $match=~s/even/\\d+_\\d+[02468]\$/;
}elsif(!($match cmp "los_0")){
  $FIFO="/tmp/unpackd_fifo_los_0";
  $match=~s/0/\\d+_\\d+[05]\$/;
}elsif(!($match cmp "los_1")){
  $FIFO="/tmp/unpackd_fifo_los_1";
  $match=~s/1/\\d+_\\d+[16]\$/;
}elsif(!($match cmp "los_2")){
  $FIFO="/tmp/unpackd_fifo_los_2";
  $match=~s/2/\\d+_\\d+[27]\$/;
}elsif(!($match cmp "los_3")){
  $FIFO="/tmp/unpackd_fifo_los_3";
  $match=~s/3/\\d+_\\d+[38]\$/;
}elsif(!($match cmp "los_4")){
  $FIFO="/tmp/unpackd_fifo_los_4";
  $match=~s/4/\\d+_\\d+[49]\$/;
}elsif(!($match cmp "los")){
  $FIFO="/tmp/unpackd_fifo_los";
  $match.="_\\d+_\\d+\$";
}elsif (!($match cmp "fast_tdrss")){
  $FIFO="/tmp/unpackd_fifo_sip_hr";
  $match.="_\\d+_\\d+\$";
}elsif (!($match cmp "slow_tdrss")){
  $FIFO="/tmp/unpackd_fifo_sip_lr";
  $match.="_\\d+_\\d+\$";
}elsif (!($match cmp "iridium")){
  $FIFO="/tmp/unpackd_fifo_iridium";
  $match.="_\\d+_\\d+\$";
}elsif (!($match cmp "openport_even")){
  $FIFO="/tmp/unpackd_fifo_openport_even";
  $match=~s/even/\\d+_\\d+[02468]\$/;
}elsif (!($match cmp "openport_odd")){
  $FIFO="/tmp/unpackd_fifo_openport_odd";
  $match=~s/odd/\\d+_\\d+[13579]\$/;
}elsif (!($match cmp "openport")){
  $FIFO="/tmp/unpackd_fifo_openport";
  $match.="_\\d+_\\d+\$";
}elsif(!($match cmp "los_a")){
  $FIFO="/tmp/unpackd_fifo_los_a";
  $match=~s/a/\\d+_\\d+0\$/;
}elsif(!($match cmp "los_b")){
  $FIFO="/tmp/unpackd_fifo_los_b";
  $match=~s/b/\\d+_\\d+1\$/;
}elsif(!($match cmp "los_c")){
  $FIFO="/tmp/unpackd_fifo_los_c";
  $match=~s/c/\\d+_\\d+2\$/;
}elsif(!($match cmp "los_d")){
  $FIFO="/tmp/unpackd_fifo_los_d";
  $match=~s/d/\\d+_\\d+3\$/;
}elsif(!($match cmp "los_e")){
  $FIFO="/tmp/unpackd_fifo_los_e";
  $match=~s/e/\\d+_\\d+4\$/;
}elsif(!($match cmp "los_f")){
  $FIFO="/tmp/unpackd_fifo_los_f";
  $match=~s/f/\\d+_\\d+5\$/;
}elsif(!($match cmp "los_g")){
  $FIFO="/tmp/unpackd_fifo_los_g";
  $match=~s/g/\\d+_\\d+6\$/;
}elsif(!($match cmp "los_h")){
  $FIFO="/tmp/unpackd_fifo_los_h";
  $match=~s/h/\\d+_\\d+7\$/;
}elsif(!($match cmp "los_i")){
  $FIFO="/tmp/unpackd_fifo_los_i";
  $match=~s/i/\\d+_\\d+8\$/;
}elsif(!($match cmp "los_j")){
  $FIFO="/tmp/unpackd_fifo_los_j";
  $match=~s/j/\\d+_\\d+9\$/;
}else{
  die "$$: Unknown match argument.";
}

# Since this is a daemon, we put it in a topmost infinite loop
while(1){
  # Open directory to monitor
  # Directory entries are actually links which are placed by data xfer
  # daemon
  my @links;
  if(-e $link_dir){
    opendir DIR,$link_dir;
    @links = grep /$match/,readdir DIR;
    closedir DIR;
  }
  if(@links==0){
    sleep 1;
    next;
  }
  for my $l (@links){
    my $now=time();
    print STDERR "$$: Processing $l @ $now\n";
    # Send data to a pipe
    system "cat $link_dir/$l > $FIFO";
    # Remove link after processing
    system "rm -f $link_dir/$l";
  }
}
