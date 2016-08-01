#!/usr/bin/perl
# -*- perl -*-
# Current status page for ANITA monitoring web pages.
# PM: 08/15/2005
# $Header$

use strict;

#$ENV{PATH}   = '/bin:/usr/bin';
use DBI;
use Time::Local;
require 'anitagse.pi';


# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
print <<HEAD;
Content-Type: text/html

<html>
<body>
HEAD

# Make connection to DB and retrieve latest info
my $dbh=DBI->connect("DBI:Pg:dbname=emflight");
if($dbh){
  my $sth=$dbh->prepare("SELECT evnum,time FROM hd ORDER BY time DESC LIMIT 10");
  print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  print "<form method=\"POST\" action=\"event.cgi\">\n";
  while(my $r = $sth->fetchrow_hashref){
    my $evnum=$r->{'evnum'};
    my $timestr=timestr(gmtime($r->{'time'}));
    print "Evnum: <input type=submit name=evnum value=$evnum> @ $timestr <br>\n"
  }
  $sth->finish;
  $dbh->disconnect();
}else {
  print "Cannot connect to Postgres server: $DBI::errstr\n";
  print " db connection failed\n";
}

print  <<TAIL;
</form>
</body>
</html>
TAIL


