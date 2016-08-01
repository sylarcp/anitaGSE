#!/usr/bin/perl
# -*- perl -*-
# Title page for ANITA monitoring web pages. Display basic info status.
# PM: 08/12/2005
# $Header$

use strict;

require 'anitagse.pi';
require 'AnitaLocalConfig.pi';
use vars qw($link_dir);

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
<table width=100% rules=all frame=box>
<tr>
<td valign=top width=55%>
<b><font size=+1>ANITA GSE Monitor</font><br>
running on $host</b><br>
Current GMT: $gmt<br>
gse version: 3.0.1 - svn: 1.6.11 [3062]<br>
Postgres version: 8.4.20
</td>
HEAD

my $link_files=`ls -1 $link_dir | wc -l`;
my $old_link_files=`ls -1 $link_dir/old | wc -l`;
my $gui_conections=`ps -U postgres w | grep gui | wc -l`;
# my @dbs=`psql template1 -c "select datname from pg_database order by datname desc;" | grep anita`;
my @dbs=`psql -A -F ' - ' template1 -c "select datname,pg_size_pretty(pg_database_size(datname)) from pg_database where datname[6]='0' order by datname desc;" | grep anita ; psql -A -F ' - ' template1 -c "select datname,pg_size_pretty(pg_database_size(datname)) from pg_database where datname[6]='1' order by datname desc;" | grep anita`;
chomp $link_files;
chomp $old_link_files;
chomp $gui_conections;
print "<td valign=top width=25%>GUI connections: $gui_conections<br>\n";
print "Files in processing dir: ",$link_files-1,"<br>\n";
print "Files skipped: $old_link_files</td>\n";
print "<td valign=top width=20%><b><u>DBs available</u></b><br>\n";
foreach my $db (@dbs){
  chomp $db;
  print "$db<br>\n";
}
print "</td>\n";
print <<TAIL;
</tr>
</table>
</center>
</body>
</html>
TAIL
