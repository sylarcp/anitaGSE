#!  /usr/bin/perl
# -*- perl -*-

# Current status page for ANITA monitoring web pages.
# PM: 08/15/2005
# $Header$

use strict;

use CGI;
use Time::Local;
require 'anitagse.pi';

# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
my $refrate=60;  # Refresh rate in seconds
print <<HEAD;
Content-Type: text/html
Refresh: $refrate;status.cgi

<html>
<head><base target=\"status\"</head>
<body>
HEAD

# Open GPS position file and make table
open GPSPOS,"/tmp/anita_gpsloc.txt";
my ($pcktime,$tod,$lat,$lon,$alt)=split /\s+/,<GPSPOS>;
close GPSPOS;
print "<table border=1>\n<tr><td valign=top colspan=2><b>GPS Position</b></td>\n";
print "<tr><td>UTC</td><td>",timestr(gmtime($pcktime)),"</td>\n";
print "<tr><td>ToD</td><td>",gpstimestr($tod),"</td>\n";
print "<tr><td>Lat</td><td>",$lat,"</td>\n";
print "<tr><td>Lon</td><td>",$lon,"</td>\n";
print "<tr><td>Alt</td><td>",$alt,"</td>\n";
print "</table>\n";

# Open GPS orientation file and make table
open GPSORIENT,"/tmp/anita_gpsorient.txt";
my ($pcktime,$head,$pitch,$roll)=split /\s+/,<GPSORIENT>;
close GPSORIENT;
print "<table border=1>\n<tr><td valign=top colspan=2><b>GPS Orientation</b></td>\n";
print "<tr><td>UTC</td><td>",timestr(gmtime($pcktime)),"</td>\n";
print "<tr><td>Head</td><td>",$head,"</td>\n";
print "<tr><td>Pitch</td><td>",$pitch,"</td>\n";
print "<tr><td>Roll</td><td>",$roll,"</td>\n";
print "</table>\n";

# Open critical HK file and make table
open HKCRIT,"/tmp/anita_hkcrit.txt";
my ($pcktime,$toti,$pvv,$tsbs,$tplate)=split /\s+/,<HKCRIT>;
close HKCRIT;
print "<table border=1>\n<tr><td valign=top colspan=2><b>Critical HK</b></td>\n";
print "<tr><td>UTC</td><td>",timestr(gmtime($pcktime)),"</td>\n";
print "<tr><td>Tot I</td><td>",$toti,"</td>\n";
print "<tr><td>PV V</td><td>",$pvv,"</td>\n";
print "<tr><td>T cpu</td><td>",$tsbs,"</td>\n";
print "<tr><td>T plt</td><td>",$tplate,"</td>\n";
print "</table>\n";

# Open command echo file and make table
open CMDECHO,"/tmp/anita_cmdecho.txt";
my ($pcktime,$good,@cmdstr)=split /\s+/,<CMDECHO>;
close CMDECHO;
print "<table border=1>\n<tr><td valign=top colspan=2><b>Last command</b></td>\n";
print "<tr><td>UTC</td><td>",timestr(gmtime($pcktime)),"</td>\n";
print "<tr><td>Good</td><td>",$good,"</td>\n";
print "<tr><td>Cmd</td><td>",join(' ',@cmdstr),"</td>\n";
print "</table>\n";

print  <<TAIL;
</table>
</body>
</html>
TAIL

