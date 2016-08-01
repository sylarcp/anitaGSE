#!/usr/bin/perl

# Receive data from Tiger, fan out as necessary, and untar it. List of untarred
# file is sent to a pipe (which should be linked to /dev/null if no one is
# listening, otherwise script will block)
#
# $Source: /cvsroot/anita-lite/gse/tcp/receive_data.pl,v $
# $Date: 2003/12/14 04:32:35 $
# $Revision: 1.8 $
# $Author: predragm $
#

use Cwd;
use File::Path;
use File::Basename;
use File::Temp;
use POSIX "sys_wait_h";

# Extract our path that called this script and appenind it to @INC
($name, $path, $suffix) = fileparse($0,'');
push @INC, $path;
require 'AnitaLocalConfig.pi';

# Make specified directories if needed
mkpath($target_dir, 0, 0755) unless (-e $target_dir);
mkpath($fanout_dir, 0, 0755) unless (-e $fanout_dir);

# Select a temporary file name to save the incoming file
$newFile=mktemp("$fanout_dir/temp_XXXXXX");
open TAROUT, ">$newFile";

# Pipe our input stream into temporary fanout file
binmode STDIN; # just to be on a safe side...
binmode TAROUT;
while(<STDIN>){print TAROUT;}
close TAROUT;

# Fanout the temporary file
my @childpid;
foreach $ip (@fanoutList){
  my $pid;
  print STDERR "$$: Sending $newFile to $ip\n";
  unless($pid=fork){   # Fork this out, so that we don't have to wait
    system("$tcpclient -R -v $ip $port sh -c \"cat $newFile 1>&7\" 1>&2");
    exit;
  }
  push @childpid,$pid;
}
# Now untar the data and catch the filenames
open FILES,"tar xvCf $target_dir $newFile |";
@fileList = grep /\/\d+$/, <FILES>;
close FILES;

# Send fileList to the output pipe
use FileHandle;
open FILE_PIPE,">>$filename_pipe";
autoflush FILE_PIPE 1;
for $f (@fileList){
  print FILE_PIPE $f;
  print STDERR "$$: Received $f"; # Log receiption
}
close FILE_PIPE;

# Wait for file sending to complete and then erase 
foreach my $pid (@childpid){
  waitpid $pid,0;
}
unlink $newFile; # Remove now that all children exited


