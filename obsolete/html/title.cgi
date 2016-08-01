#!  /usr/bin/perl
# -*- perl -*-
# Title page for ANITA monitoring web pages. Display basic info status.
# PM: 08/12/2005
# $Header$

use strict;

#$ENV{PATH}   = '/bin:/usr/bin';
use DBI;
require 'anitagse.pi';

# Print HTML header information, title, etc.
my $refrate=20;  # Title refresh rate in seconds
my $host=`hostname`;
chomp $host;
my $gmt=timestr(gmtime(time));
print <<HEAD;
Content-Type: text/html
Refresh: $refrate

<html>
<body>
<center>
<table cellpadding="20%">
<tr>
<td align="center">
<h2>ANITA Flight Monitor</h2>
<b>running on $host</b><br>
Current GMT: $gmt<br>
HEAD

my $files=`ls -1 /mnt/data/emflight/link/d* | wc -l`;
chomp $files;
print "$files files lag in processing</td><td>\n";

# Make connection to DB and retrieve latest packet times
my $dbh=DBI->connect("DBI:Pg:dbname=emflight");
if($dbh){
  my $table;
  for $table (("hd","hkd_raw","gpsd_pat","gpsd_sat","cmd")){
    my $sth = $dbh->prepare("SELECT max(time) FROM $table");
    print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
    print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
    my $pcktime=timestr(gmtime($sth->fetchrow));
    my $tabname=uc $table;
    print "Last $tabname:   $pcktime<br>\n";
    $sth->finish;
  }
  $dbh->disconnect();
} else {
   print "Cannot connect to Postgres server: $DBI::errstr\n";
   print " db connection failed\n";
}


print <<TAIL;
</td>
</tr>
</table>
</center>
</body>
</html>
TAIL
