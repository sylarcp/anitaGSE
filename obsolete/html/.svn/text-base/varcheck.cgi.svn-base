#!/usr/bin/perl
# -*- perl -*-
# Current status page for ANITA monitoring web pages.
# PM: 08/15/2005
# $Header$

use strict;

#$ENV{PATH}   = '/bin:/usr/bin';
use CGI;
use DBI;
use GD::Graph::lines;
use POSIX qw(ceil floor);
use Time::Local;
require 'anitagse.pi';

# Get calling parameters
my $cgi = new CGI;
my $time = $cgi->param("time");
my $last = $cgi->param("last");
my $start = $cgi->param("start");
my $end = $cgi->param("end");
my @vars=grep /:/,$cgi->param; # Select variables

# Recreate query string for refresh
my $time_string="time=$time&last=$last&start=$start&end=$end";
my $query_string="";
foreach my $var (@vars){
  $query_string.="$var=on&";
}

# Keep track of temporary files
my @delete_list=();

# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
my $refrate=300;  # Refresh rate in seconds
print <<HEAD;
Content-Type: text/html
Refresh: $refrate;varcheck.cgi?$time_string&$query_string

<html>
<body>
HEAD

if ($time>0){
  $end=time;
#  $end=1124313440;
  $start=$end-$time;
}elsif($time==-1){
  $end=time;
#  $end=1124313440;
  $start=1;
}elsif($time==-2){
  $end=time;
#  $end=1124313440;
  $start=$end-$last;
}elsif($time==-3){
  $start=gettime($start) if($start);
  $end=gettime($end) if($end);
}

if($start and $end){
  # Now prepare start/end time navigation buttons
  print "<table border=1><tr><td><b>Start time</b></td><td><b>End time</b></td></tr>\n";
  print "<tr><td><table>\n";
  my @offsets=("-600:-10min","-1800:-30min","-21600:-6hr","600:+10min","1800:+30min","21600:+6hr");
  my $i=0;
  foreach my $off (@offsets){
    ++$i;
    my ($dt,$lbl)=split /:/,$off;
    print "<tr>" if ($i%3==1);
    print "<td><form action=varcheck.cgi method=\"post\">\n";
    print "<input type=\"hidden\" name=\"time\" value=\"-4\">\n";
    print "<input type=\"hidden\" name=\"end\" value=\"$end\">\n";
    my $new_start=$start+$dt;
    print "<input type=\"hidden\" name=\"start\" value=\"$new_start\">\n";
    foreach my $var (@vars){
      print "<input type=\"hidden\" name=\"$var\" value=\"on\">\n";
    }
    print "<input type=\"submit\" value=\"$lbl\">\n";
    print "</form></td>\n";
    print "</tr>" if ($i%3==0);
  }
  print "</tr></table></td><td><table><tr>\n";
  $i=0;
  foreach my $off (@offsets){
    ++$i;
    my ($dt,$lbl)=split /:/,$off;
    print "<tr>" if ($i%3==1);
    print "<td><form action=varcheck.cgi method=\"post\">\n";
    print "<input type=\"hidden\" name=\"time\" value=\"-4\">\n";
    my $new_end=$end+$dt;
    print "<input type=\"hidden\" name=\"end\" value=\"$new_end\">\n";
    print "<input type=\"hidden\" name=\"start\" value=\"$start\">\n";
    foreach my $var (@vars){
      print "<input type=\"hidden\" name=\"$var\" value=\"on\">\n";
    }
    print "<input type=\"submit\" value=\"$lbl\">\n";
    print "</form></td>\n";
    print "</tr>" if ($i%3==0);
  }
  print "</tr></table></td></tr></table>\n";

  my $startstr=timestr(gmtime($start));
  # Make connection to DB and retrieve latest info
  my $dbh=DBI->connect("DBI:Pg:dbname=emflight");
  if($dbh){
    foreach my $var (@vars){
      my ($tbl,$v,$lbl,$fmt)=split /:/,$var;
      my $sth = $dbh->prepare("SELECT time,$v FROM $tbl WHERE (time>=$start AND time<=$end) ORDER BY time");
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
      my ($data_ref,$reftime)=makedata($sth,"time",(split /\[/,$v)[0]);
      $startstr=timestr(gmtime($reftime));
      my $graph = GD::Graph::lines->new(400, 200);
      my $maxx=(int($data_ref->[0][-1]/10)+1)*10;
      $graph->set(x_label           => 't [min]',
		  y_label           => "$lbl",
		  title             => "t=0 @ $startstr",
#		y_max_value       => 30000,
#		y_min_value       => 0,
		  y_tick_number     => 10,
		  y_label_skip      => 1,
		  y_number_format   => "%$fmt",
		  x_min_value       => 0,
		  x_max_value       => $maxx,
		  x_tick_number     => 10,
		  x_label_skip      => 2,
		  x_number_format   => "%.1f",
#		  x_tick_offset     => 0,
		  r_margin          => 10
		 ) or print "$graph->error<br>\n";
      $graph->set_legend($var);
      my $gd;
      if(@{$data_ref->[0]}){
	# Need to fix a bug where plot jams if all y are equal
	my $miny=1e30;
	my $maxy=-1e30;
	for(my $i=1;$i<@$data_ref;++$i){
	  foreach my $y (@{$data_ref->[$i]}){
	    $miny=$y if $y<$miny;
	    $maxy=$y if $y>$maxy;
	  }
	}
	$data_ref->[1][-1]+=1e-6 if($miny==$maxy);
	$graph->set(y_max_value       => ($maxy+0.1),
		    y_min_value       => ($miny-0.1));
	$gd=$graph->plot($data_ref) or print $graph->error;
      }else{
	my @dummy=([(0,1)],[(0,0)]);
	$gd=$graph->plot(\@dummy) or print $graph->error;
      }
      my $tmpfile="tmp/$v-$$.jpg";
      open(IMG, ">$tmpfile") or print "$!<br>\n";
      binmode IMG;
      print IMG $gd->jpeg or print "$graph->error<br>\n";
      close(IMG);
      print  "<img src=\"$tmpfile\"><br>\n";
      push @delete_list,$tmpfile;

      $sth->finish;
    }

    $dbh->disconnect();
  } else {
    print "Cannot connect to Postgres server: $DBI::errstr\n";
    print " db connection failed\n";
  }
}else{
  print "ERROR: Malformated request!<br>\n";
}

print  <<TAIL;
</body>
</html>
TAIL

# Now forkout and erase tmp files after timeout period
exit if (fork);
sleep $refrate+5;
foreach my $f (@delete_list){
  unlink $f;
}
