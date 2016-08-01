#!/usr/bin/perl

# Daemon which monitors directory with new packet links, retires old unprocessed links and 
# resurrects them as needed

# $Header$

use strict;

my $age_cut=$ARGV[0];
die "$$: Must give age parameter for retiring links." unless $age_cut;
my $link_dir='.';
$link_dir=$ARGV[1] if $ARGV[1];
my $link_old="$link_dir/old";
print STDERR "$$: Monitoring $link_dir\n";

#my $resurrect_count=1; # Ressurect up to 1 links
my $resurrect_count=10; # Ressurect up to 1 links//amirs additon

# Since this is a daemon, we put it in a topmost infinite loop
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
#if(@links<=1){ # Few links so resurrect some old ones
 if(@links<=10){ # Few links so resurrect some old ones//amirs addition
    my @old_links;
    if(-e $link_old){
      opendir DIR,$link_old;
      @old_links = grep /_\d+$/,readdir DIR;
      closedir DIR;
    }
    my $count=0;
    for my $l (@old_links){
      my $now=time();
      print STDERR "$$: Resurrecting $l @ $now\n";
      system("cp -d $link_old/$l $link_dir");
      system("rm $link_old/$l");
      last if(++$count>=$resurrect_count);
    }
    sleep 3;
    next;
  }	
  for my $l (@links){
    my $now=time();
    my @filestat=lstat "$link_dir/$l";
    my $age=$now-$filestat[10]; # Use access time for age calculation, since it could be one being processed
    if($age>=$age_cut){  # Retire link
      print STDERR "$$: Moving $l @ $now\n";
      system "mv $link_dir/$l $link_old";
    }
  }
  # Wait a bit and then loop again
  sleep 1;
}
