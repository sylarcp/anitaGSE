#!  /usr/bin/perl
# -*- perl -*-

# Current status page for ANITA monitoring web pages.
# PM: 08/15/2005
# $Header$

use strict;

#$ENV{PATH}   = '/bin:/usr/bin';
use CGI;
use DBI;
use GD::Graph::lines;
use Time::Local;
require 'anitagse.pi';

# Get calling parameters
my $cgi = new CGI;
my $time = $cgi->param("time");

# Keep track of tmp files we make
my @delete_list=();
# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
my $refrate=300;  # Refresh rate in seconds
print <<HEAD;
Content-Type: text/html
Refresh: $refrate;status.cgi?time=$time

<html>
<body>
HEAD

if($time>0){
  print "<h3>Status at GMT $time</h3>\n";
}else{
  print "<h3>Current status</h3>\n";
}

if($time>0){
  $time=gettime($time) if($time);
}
# Make connection to DB and retrieve latest info
my $dbh=DBI->connect("DBI:Pg:dbname=emflight");
if($dbh){
  my $sth;
  print "<table>\n";

  # Command
  if($time==-1){
    $sth = $dbh->prepare("SELECT * FROM cmd WHERE time=(SELECT max(time) FROM cmd)");
  }else{
    $sth = $dbh->prepare("SELECT * FROM cmd WHERE time>=$time ORDER BY time LIMIT 1");
  }
  print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  print "<tr><td valign=\"top\"><b>Last Command</b>\n";
  if(my $r = $sth->fetchrow_hashref){
    print "<table>\n";
    print "<tr><td>Time</td><td>",timestr(gmtime($r->{'time'})),"</td></tr>\n";
    print "<tr><td>Flag</td><td>",$r->{'flag'},"</td></tr>\n";
    print "<tr><td>Bytes</td><td>",$r->{'bytes'},"</td></tr>\n";
    my $val=substr $r->{'cmd'},1,-1; # Strips {}
    my @cmd=split /,/,$val;
    my $cmdstr="";
    foreach my $c (@cmd){
      $cmdstr.=sprintf(" %3d ",$c);
    }
    print "<tr><td>Cmd</td><td>$cmdstr</td></tr>\n";
    print "</table>\n";
  }
  print "</td>";
  $sth->finish;

  # Get GPS Position
  if($time==-1){
    $sth = $dbh->prepare("SELECT * FROM gpsd_pat WHERE time=(SELECT max(time) FROM gpsd_pat)");
  }else{
    $sth = $dbh->prepare("SELECT * FROM gpsd_pat WHERE time>=$time ORDER BY time LIMIT 1");
  }
  print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  print "<tr><td valign=\"top\"><b>GPS Position</b><br>\n";
  if(my $r = $sth->fetchrow_hashref){
    print "<table>\n";
    print "<tr><td>Time</td><td>",timestr(gmtime($r->{'time'})),"</td></tr>\n";
    print "<tr><td>GPS TOD</td><td> ",gpstimestr($r->{'tod'}),"<br>\n";
    print "<tr><td>Flag</td><td> ",$r->{'flag'},"</td></tr>\n";
    print  "<tr><td>Latitude</td><td> ",$r->{'latitude'}," deg </td></tr>\n";
    print  "<tr><td>Longitude</td><td> ",$r->{'longitude'}," deg</td></tr>\n";
    print  "<tr><td>Altitude</td><td> ",$r->{'altitude'}," m</td></tr>\n";
    print  "<tr><td>Heading</td><td> ",$r->{'heading'}," deg</td></tr>\n";
    print  "<tr><td>Pitch</td><td> ",$r->{'pitch'}," deg</td></tr>\n";
    print "<tr><td>Roll</td><td> ",$r->{'roll'}," deg</td></tr>\n";
    print "<tr><td>Mrms</td><td> ",$r->{'mrms'}," m</td></tr>\n";
    print "<tr><td>Brms</td><td> ",$r->{'brms'}," m</td></tr>\n";
    print "</table>\n";
  }
  print "</td>";
  $sth->finish;

  # Get GPS satellite info
  if($time==-1){
    $sth = $dbh->prepare("SELECT * FROM gpsd_sat WHERE time=(SELECT max(time) FROM gpsd_sat)");
  }else{
    $sth = $dbh->prepare("SELECT * FROM gpsd_sat  WHERE time>=$time ORDER BY time LIMIT 1");
  }
  print  "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print  "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  print "<td valign=top colspan=2><b>GPS Satellite Info</b><br>\n";
  if(my $r = $sth->fetchrow_hashref){
    print "<table>\n";
    print "<tr><td>Time</td><td>",timestr(gmtime($r->{'time'})),"</td></tr>\n";
    print "<tr><td>Nsat</td><td>",$r->{'numsats'},"</td></tr>\n";
    my @satinfo=getsatinfo($r->{'satinfo'});
    print "<tr><td colspan=2><table border=1>\n";
    for(my $i=0;$i<int($r->{'numsats'});++$i){
      if($i%3==0){
	print "<tr><td><table>\n";
      }else{
	print "<td><table>\n";
      }
      print "<tr><td>PRN</td><td>",$satinfo[$i][0],"</td></tr>\n";
      print "<tr><td>Azimuth</td><td>",$satinfo[$i][1]," deg</td></tr>\n";
      print "<tr><td>Elevation</td><td>",$satinfo[$i][2]," deg</td></tr>\n";
      print "<tr><td>SNR</td><td>",$satinfo[$i][3],"</td></tr>\n";
      print "<tr><td>Flag</td><td>",$satinfo[$i][4],"</td></tr>\n";
      if($i%3==2){
	print "</table></td></tr>\n";
      }else{
	print "</table></td>\n";
      }
    }
    print "</table></td><tr>\n";
    print "</table>\n";
  }
  print "</td></tr>\n";
  $sth->finish;

  # Get housekeeping data
  if($time==-1){
    $sth = $dbh->prepare("SELECT * FROM hkd WHERE time=(SELECT max(time) FROM hkd) AND us=(SELECT max(us) FROM hkd WHERE time=(SELECT max(time) FROM hkd))");
  }else{
    $sth = $dbh->prepare("SELECT * FROM hkd WHERE time>=$time ORDER BY time LIMIT 1");
  }
  print  "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print  "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  print "<tr><td colspan=2><b>Housekeeping Info</b></td></tr>\n";
  if(my $r=$sth->fetchrow_hashref){
    print "<tr><td valign=top><table border=1>\n";
    print "<tr><td>Time</td><td>",timestr(gmtime($r->{'time'})),"</td></tr>\n";
    print "<tr><td>Microsec</td><td>",$r->{'us'},"</td></tr>\n";
    print "<tr><td>Accel 1</td><td>",join(" g<br>",($r->{'acc1x'},$r->{'acc1y'},$r->{'acc1z'}))," g</td></tr>\n";
    print "<tr><td>Accel 2</td><td>",join(" g<br>",($r->{'acc2x'},$r->{'acc2y'},$r->{'acc2z'}))," g</td></tr>\n";
    print "<tr><td>Sun Sens</td><td>",join(" X<br> ",($r->{'ssx'},$r->{'ssy'}))," Y</td></tr>\n";
    print "<tr><td>Accel 1 T</td><td>",$r->{'acc1t'}," C </td></tr>\n";
    print "<tr><td>Accel 2 T</td><td>",$r->{'acc2t'}," C </td></tr>\n";
    print "<tr><td>Sun Sens T</td><td>",$r->{'sst'}," C </td></tr>\n";
    print "<tr><td>Pressure hi</td><td>",$r->{'pressh'},"  PSI</td></tr>\n";
    print "<tr><td>Pressure lo</td><td>",$r->{'pressl'},"  Torr</td></tr>\n";
    print "<tr><td>SBS T</td><td>",join(" C<br>",($r->{'sbst1'},$r->{'sbst2'}))," C</td></tr>\n";
    print "<tr><td>Mag</td><td>",join(" G<br>",($r->{'magx'},$r->{'magy'},$r->{'magz'}))," G</td></tr>\n";
    print "</table></td><td valign=top><table border=1>\n";
    print "<tr><td>+1.5 V</td><td>",join(" ",($r->{'p1_5v'},'V <br>',$r->{'p1_5i'},'A')),"</td></tr>\n";
    print "<tr><td>+2.5 V</td><td>",join(" ",($r->{'p2_5v'},'V <br>',$r->{'p2_5i'},'A')),"</td></tr>\n";
    print "<tr><td>+3.3 V</td><td>",join(" ",($r->{'p3_3v'},'V <br>',$r->{'p3_3i'},'A')),"</td></tr>\n";
    print "<tr><td>+5 V</td><td>",join(" ",($r->{'p5v'},'V <br>',$r->{'p5i'},'A')),"</td></tr>\n";
    print "<tr><td>+12 V</td><td>",join(" ",($r->{'p12v'},'V <br>',$r->{'p12i'},'A')),"</td></tr>\n";
    print "<tr><td>+12IP V</td><td>",join(" ",($r->{'p12ipv'},'V <br>',$r->{'p12ipi'},'A')),"</td></tr>\n";
    print "<tr><td>+24 V</td><td>",join(" ",($r->{'batv'},'V <br>',$r->{'p24i'},'A')),"</td></tr>\n";
    print "<tr><td>+PV V</td><td>",join(" ",($r->{'ppvv'},'V <br>',$r->{'ppvi'},'A')),"</td></tr>\n";
    print "<tr><td>-5 V</td><td>",join(" ",($r->{'n5v'},'V <br>',$r->{'n5i'},'A')),"</td></tr>\n";
    print "<tr><td>-12 V</td><td>",join(" ",($r->{'n12v'},'V <br>',$r->{'n12i'},'A')),"</td></tr>\n";
    print "<tr><td>GPS V</td><td>",$r->{'gpsv'}," V</td></tr>\n";
    print "<tr><td>RFCM V</td><td>",$r->{'rfcmv'}," V</td></tr>\n";
    print "<tr><td>Bat. I</td><td>",$r->{'bati'}," A</td></tr>\n";
    print "</table></td><td valign=top><table border=1>\n";
    print "<tr><td>T 1-5</td><td>",join(" : ",($r->{'t1'},$r->{'t2'},$r->{'t3'},$r->{'t4'},$r->{'t5'}))," C</td></tr>\n";
    print "<tr><td>T 6-10</td><td>",join(" : ",($r->{'t6'},$r->{'t7'},$r->{'t8'},$r->{'t9'},$r->{'t10'}))," C</td></tr>\n";
    print "<tr><td>T 11-15</td><td>",join(" : ",($r->{'t11'},$r->{'t12'},$r->{'t13'},$r->{'t14'},$r->{'t15'}))," C</td></tr>\n";
    print "<tr><td>T 16-20</td><td>",join(" : ",($r->{'t16'},$r->{'t17'},$r->{'t18'},$r->{'t19'},$r->{'t20'}))," C</td></tr>\n";
    print "<tr><td>T 21-25</td><td>",join(" : ",($r->{'t21'},$r->{'t22'},$r->{'t23'},$r->{'t24'},$r->{'t25'}))," C</td></tr>\n";
    print "<tr><td>T 26-30</td><td>",join(" : ",($r->{'t26'},$r->{'t27'},$r->{'t28'},$r->{'t29'},$r->{'t30'}))," C</td></tr>\n";
    print "<tr><td>T 31-35</td><td>",join(" : ",($r->{'t31'},$r->{'t32'},$r->{'t33'},$r->{'t34'},$r->{'t35'}))," C</td></tr>\n";
    print "<tr><td>T 36-40</td><td>",join(" : ",($r->{'t36'},$r->{'t37'},$r->{'t38'},$r->{'t39'},$r->{'t40'}))," C</td></tr>\n";
    print "<tr><td>T 41-45</td><td>",join(" : ",($r->{'t41'},$r->{'t42'},$r->{'t43'},$r->{'t44'},$r->{'t45'}))," C</td></tr>\n";
    print "<tr><td>T 46-50</td><td>",join(" : ",($r->{'t46'},$r->{'t47'},$r->{'t48'},$r->{'t49'},$r->{'t50'}))," C</td></tr>\n";
    print "</table></td></tr>\n";
  }

  $sth->finish;

  # Get event
  if($time==-1){
    $sth = $dbh->prepare("SELECT * FROM hd WHERE evnum=(SELECT last FROM shortcut WHERE ref=1)");
  }else{
    $sth = $dbh->prepare("SELECT * FROM hd WHERE time>=$time ORDER BY time LIMIT 1");
  }
  print  "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print  "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  print "<td colspan=2><b>Last event</b></td></tr>\n";
  my $evnum;
  # Header info
  if(my $r = $sth->fetchrow_hashref){
    $evnum=$r->{'evnum'};
    print "<tr><td valign=\"top\"><table>\n";
    print "<tr><td>Event #</td><td>",$r->{'evnum'},"</td></tr>\n";
    print "<tr><td>Time</td><td>",timestr(gmtime($r->{'time'})),"</td></tr>\n";
    print "<tr><td>Microsec</td><td>",$r->{'us'},"</td></tr>\n";
    print "<tr><td>GPS ns</td><td>",$r->{'ns'},"</td></tr>\n";
    print "<tr><td>Calib</td><td>",$r->{'calib'},"</td></tr>\n";
    print "<tr><td>Priority</td><td>",$r->{'priority'},"</td></tr>\n";
    print "<tr><td>TrigTime</td><td>",$r->{'trigtime'},"</td></tr>\n";
    print "<tr><td>TrigNum</td><td>",$r->{'trignum'},"</td></tr>\n";
    # Interpret trigger
    my $trigtype=$r->{'trigtype'};
    my $mask=($trigtype&0xf0)>>4;
    my $trig=($trigtype&0x0f);
    my @maskstr;
    my @trigstr;
    my %trigcode=(1 => 'RF',2 => 'ADU5',4 => 'G12',8 => 'SFT');
    for(my $i=1;$i<=8;$i*=2){
      push @maskstr,$trigcode{$i} if($mask & $i);
      push @trigstr,$trigcode{$i} if($trig & $i);
    }
    print "<tr><td valign=top>TrigType</td><td><table border=1>";
    print "<tr><td>Mask</td><td>Trig</td></tr>";
    print "<tr><td>",join("<br>",@maskstr),"</td><td valign=top>",join("<br>",@trigstr),"</td></tr>";
    print "</table></td></tr>\n";
    print "<tr><td>PPS</td><td>",$r->{'ppsnum'},"</td></tr>\n";
    print "<tr><td>c3po</td><td>",$r->{'c3ponum'},"</td></tr>\n";
    print "<tr><td>other</td><td>",$r->{'otherbits'},"</td></tr>\n";
    print "</table>\n";
    print "</td>\n";
  }
  $sth->finish;

  if($evnum){
    $sth = $dbh->prepare("SELECT * FROM wvem WHERE evnum=$evnum");
    print  "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
    print  "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  }
  if(my $r = $sth->fetchrow_hashref){
    # Waveforms
    print "<td colspan=2><table>\n";
    my $i=0;
    my @chlist=("h1","v1","h2","v2","veto_h1","veto_v1","veto_h2","veto_v2");
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
                  y_number_format   => '%.3f',
		  x_max_value       => 85,
		  x_tick_number     => 17,
#		  x_label_skip      => 2,
#		  x_tick_offset     => 0,
		  r_margin          => 10
		 ) or die $graph->error;
      my $gd = $graph->plot(\@data) or die $graph->error;
#      system("rm -f tmp/ch$i.png");
      my $tmpfile="tmp/$ch-$$.jpg";
      open(IMG, ">$tmpfile") or die $!;
      binmode IMG;
      print IMG $graph->plot(\@data)->jpeg;
      close(IMG);
      print "<tr>" if ($i%2);
      print  "<td><img src=\"$tmpfile\"></td>\n";
      print "</tr>" unless ($i%2);
      push @delete_list,$tmpfile;
    }
    print "</table></td>\n";
  }else{
    print "<td valign=top><b>No waveform</b></td>";
  }
  $sth->finish;
  print "</tr></table>\n";

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
