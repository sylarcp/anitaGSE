#!/usr/bin/perl

# Process TIGER binary files and make links to ASCII data for unpackd.py
# The list of files is taken from the name pipe
#
# $Source: /cvsroot/anita-lite/gse/tcp/process_data.pl,v $
# $Date: 2003/11/19 03:12:35 $
# $Revision: 1.1 $
# $Author: predragm $
#

use Cwd;
use File::Path;
use File::Basename;

# Extract our path that called this script and appenind it to @INC
($name, $path, $suffix) = fileparse($0,'');
push @INC, $path;
require 'AnitaLocalConfig.pi';

# Make specified directories if needed
mkpath($link_dir, 0, 0755) unless (-e $link_dir);

# Open pipe
open FILES,"$filename_pipe";
while(1){
  # Listen to the pipe, process named files into ascii and make links
  while(<FILES>){
    chomp;
    # Parse file name and make directories as needed
    ($name, $path, $suffix) = fileparse($_,'');
    # Make unique link name
    $linkf = $_;
    $linkf =~ s/\//_/g;
    # Should check if link already exists and generate a different name
    #while(-e "$link_dir/$linkf"){
    #  $r = int(rand 10);
    #  $linkf="${r}_$linkf";
    #}
    print STDERR "Linking $linkf\n";
    # Try to see if data file path is absolute
    if ($target_dir =~ /^\//){
      system("ln -sf $target_dir/$_ $link_dir/$linkf\n");
    }else{
      $wd=cwd();
      system("ln -sf $wd/$target_dir/$_ $link_dir/$linkf\n");
    }
  }
  sleep 1;
}
close FILES;
