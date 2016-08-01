#!  /usr/bin/perl
# -*- perl -*-

# Current status page for ANITA monitoring web pages.
# PM: 08/15/2005
# $Header$

use strict;

#$ENV{PATH}   = '/bin:/usr/bin';
#use CGI;
use DBI;
use GD::Graph::lines;
use Time::Local;
require 'anitagse.pi';

# Get calling parameters
#my $cgi = new CGI;

# Keep track of tmp files we make
my @delete_list=();
# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
my $refrate=300;  # Refresh rate in seconds
print <<HEAD;
Content-Type: text/html
Refresh: $refrate;lastrf.cgi

<html>
<body>
HEAD

# Make connection to DB and retrieve latest info
my $dbh=DBI->connect("DBI:Pg:dbname=emflight");
if($dbh){
  my $sth = $dbh->prepare("SELECT wvem.*,hd.time,hd.us,hd.trigtime,hd.c3ponum FROM wvem,hd WHERE wvem.evnum=hd.evnum ORDER BY time DESC LIMIT 5");
  print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  while(my $r = $sth->fetchrow_hashref){
    # Calculate microsec from trigtime
    my $micro=sprintf ("%010.3f",$r->{'trigtime'}/$r->{'c3ponum'}*1e6) if ($r->{'c3ponum'});
    print "<h3>Event ",$r->{'evnum'},' @ ',timestr(gmtime($r->{'time'})),".",int($r->{'us'}/1000+0.5)," trigtime=$micro us</h3><br>\n";
    print "<table>\n";
    # Waveforms
    my @chlist=("h1","v1","h2","v2","veto_h1","veto_v1","veto_h2","veto_v2");
    my $i=0;
    foreach my $ch (@chlist){
      $i++;
      my $val=substr $r->{$ch},1,-1; # Strips {}
      my @ypts=split /,/,$val;
      unless(@ypts){
	foreach my $i (1..256){
	  push @ypts,0;
	}
      }
#      my @xpts=(1..@ypts);
      my @xpts;
      for(my $x=0;$x<@ypts;++$x){push @xpts,$x/2.98;} # dt=1/2.98GHz
      my @data=([@xpts],[@ypts]);
      my $graph = GD::Graph::lines->new(350, 200);
      $graph->set(x_label           => 't [ns]',
		  y_label           => 'V [V]',
		  title             => "Channel $ch",
#		  y_max_value       => 500,
#		  y_min_value       => -500,
#		  y_tick_number     => 10,
#		  y_label_skip      => 2,
                  y_number_format   => '%.3g',
		  x_max_value       => 85,
		  x_tick_number     => 17,
#		  x_label_skip      => 2,
#		  x_tick_offset     => 0,
		  r_margin          => 10
		 ) or die $graph->error;
      my $gd = $graph->plot(\@data) or die $graph->error;
#      system("rm -f tmp/ch$i.png");
      my $tmpfile="tmp/$r->{'evnum'}_$ch-$$.jpg";
      open(IMG, ">$tmpfile") or die $!;
      binmode IMG;
      print IMG $graph->plot(\@data)->jpeg;
      close(IMG);
      print "<tr>" if ($i%2);
      print  "<td><img src=\"$tmpfile\"></td>\n";
      print "</tr>" unless ($i%2);
      push @delete_list,$tmpfile;
    }
    print "</table>\n";
  }
  $sth->finish;

  $dbh->disconnect();
} else {
   print "Cannot connect to Postgres server: $DBI::errstr\n";
   print " db connection failed\n";
}

print  <<TAIL;
</table>
</body>
</html>
TAIL

# Now forkout and erase tmp files after timeout period
exit if (fork);
sleep $refrate+5;
foreach my $f (@delete_list){
  unlink $f;
}
