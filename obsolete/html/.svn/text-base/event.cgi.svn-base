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
my $evnum = $cgi->param("evnum");

# Keep track of tmp files we make
my @delete_list=();
# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
my $refrate=300;  # Refresh rate in seconds
print <<HEAD;
Content-Type: text/html
Refresh: $refrate;event.cgi?evnum=$evnum

<html>
<body>
<h3> Event $evnum</h3>
HEAD

# Make connection to DB and retrieve latest info
my $dbh=DBI->connect("DBI:Pg:dbname=emflight");
if($dbh){
  my $sth = $dbh->prepare("SELECT * FROM hd WHERE hd.evnum=$evnum");
  print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
  print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
  if(my $r = $sth->fetchrow_hashref){
    print "<table>\n";
    print "<tr><td valign=\"top\"><b>Last Command</b>\n";
    if(my $cmdtime=$r->{'cmd'}){
      my $sthcmd=$dbh->prepare("SELECT * FROM cmd WHERE time=$cmdtime");
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sthcmd));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sthcmd->execute);
      if(my $rcmd=$sthcmd->fetchrow_hashref){
	print "<table>\n";
	print "<tr><td>Time</td><td>",timestr(gmtime($r->{'cmd'})),"</td></tr>\n";
	print "<tr><td>Flag</td><td>",$rcmd->{'flag'},"</td></tr>\n";
	print "<tr><td>Bytes</td><td>",$rcmd->{'bytes'},"</td></tr>\n";
	my $val=substr $rcmd->{'cmd'},1,-1; # Strips {}
	my @cmd=split /,/,$val;
	my $cmdstr="";
	foreach my $c (@cmd){
	  $cmdstr.=sprintf("%3d ",$c);
	}
	print "<tr><td>Cmd</td><td>$cmdstr</td></tr>\n";
	print "</table>\n";
      }
      $sthcmd->finish;
    }
    print "</td>";

    print "<tr><td valign=\"top\"><b>GPS Position</b><br>\n";
    if(my $pattime=$r->{'pat'}){
      my $sthpat=$dbh->prepare("SELECT * FROM gpsd_pat WHERE time=$pattime");
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sthpat));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sthpat->execute);
      if(my $rpat=$sthpat->fetchrow_hashref){
	print "<table>\n";
	print "<tr><td>Time</td><td>",timestr(gmtime($r->{'pat'})),"</td></tr>\n";
	print "<tr><td>GPS TOD</td><td> ",gpstimestr($rpat->{'tod'}),"\n";
	print "<tr><td>Flag</td><td> ",$rpat->{'flag'},"</td></tr>\n";
	print  "<tr><td>Latitude</td><td> ",$rpat->{'latitude'}," deg </td></tr>\n";
	print  "<tr><td>Longitude</td><td> ",$rpat->{'longitude'}," deg</td></tr>\n";
	print  "<tr><td>Altitude</td><td> ",$rpat->{'altitude'}," m</td></tr>\n";
	print  "<tr><td>Heading</td><td> ",$rpat->{'heading'}," deg</td></tr>\n";
	print  "<tr><td>Pitch</td><td> ",$rpat->{'pitch'}," deg</td></tr>\n";
	print "<tr><td>Roll</td><td> ",$rpat->{'roll'}," deg</td></tr>\n";
	print "<tr><td>Mrms</td><td> ",$rpat->{'mrms'}," m</td></tr>\n";
	print "<tr><td>Brms</td><td> ",$rpat->{'brms'}," m</td></tr>\n";
	print "</table>\n";
      }
      $sthpat->finish;
    }
    print "</td>";

    print "<td valign=top colspan=2><b>GPS Satellite Info</b>\n";
    if(my $sattime=$r->{'sat'}){
      my $sthsat=$dbh->prepare("SELECT * FROM gpsd_sat WHERE time=$sattime");
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sthsat));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sthsat->execute);
      if(my $rsat=$sthsat->fetchrow_hashref){
	print "<table>\n";
	print "<tr><td>Time</td><td>",timestr(gmtime($r->{'sat'})),"</td></tr>\n";
	print "<tr><td>Nsat</td><td>",$rsat->{'numsats'},"</td></tr>\n";
	my @satinfo=getsatinfo($rsat->{'satinfo'});
	print "<tr><td colspan=2><table border=1>\n";
	for(my $i=0;$i<int($rsat->{'numsats'});++$i){
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
	print "</table></td></tr>\n";
	print "</table>\n";
      }
      $sthsat->finish;
    }
    print "</td></tr>\n";


    print "<td colspan=2><b>Housekeeping Info</b></td></tr>\n";
    if(my $hkdtime=$r->{'hkd'}){
      my $sthhkd=$dbh->prepare("SELECT * FROM hkd WHERE time=$hkdtime AND us=$r->{'hkdus'}");
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sthhkd));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sthhkd->execute);
      if(my $rhkd=$sthhkd->fetchrow_hashref){
	print "<tr><td valign=top><table border=1>\n";
	print "<tr><td>Time</td><td>",timestr(gmtime($r->{'hkd'})),"</td></tr>\n";
	print "<tr><td>Microsec</td><td>",$r->{'hkdus'},"</td></tr>\n";
	print "<tr><td>Accel 1</td><td>",join(" g<br>",($rhkd->{'acc1x'},$rhkd->{'acc1y'},$rhkd->{'acc1z'}))," g</td></tr>\n";
	print "<tr><td>Accel 2</td><td>",join(" g<br>",($rhkd->{'acc2x'},$rhkd->{'acc2y'},$rhkd->{'acc2z'}))," g</td></tr>\n";
	print "<tr><td>Sun Sens</td><td>",join(" X<br>",($rhkd->{'ssx'},$rhkd->{'ssy'}))," Y</td></tr>\n";
	print "<tr><td>Accel 1 T</td><td>",$rhkd->{'acc1t'}," C </td></tr>\n";
	print "<tr><td>Accel 2 T</td><td>",$rhkd->{'acc2t'}," C </td></tr>\n";
	print "<tr><td>Sun Sens T</td><td>",$rhkd->{'sst'}," C </td></tr>\n";
	print "<tr><td>Pressure hi</td><td>",$rhkd->{'pressh'}," PSI</td></tr>\n";
	print "<tr><td>Pressure lo</td><td>",$rhkd->{'pressl'}," Torr</td></tr>\n";
	print "<tr><td>SBS T</td><td>",join(" C<br>",($rhkd->{'sbst1'},$rhkd->{'sbst2'})),"  C</td></tr>\n";
	print "<tr><td>Mag</td><td>",join(" G<br>",($rhkd->{'magx'},$rhkd->{'magy'},$rhkd->{'magz'}))," G</td></tr>\n";
	print "</table></td><td valign=top><table border=1>\n";
	print "<tr><td>+1.5 V</td><td>",join(" ",($rhkd->{'p1_5v'},'V<br>',$rhkd->{'p1_5i'},'A')),"</td></tr>\n";
	print "<tr><td>+2.5 V</td><td>",join(" ",($rhkd->{'p2_5v'},'V<br>',$rhkd->{'p2_5i'},'A')),"</td></tr>\n";
	print "<tr><td>+3.3 V</td><td>",join(" ",($rhkd->{'p3_3v'},'V<br>',$rhkd->{'p3_3i'},'A')),"</td></tr>\n";
	print "<tr><td>+5 V</td><td>",join(" ",($rhkd->{'p5v'},'V<br>',$rhkd->{'p5i'},'A')),"</td></tr>\n";
	print "<tr><td>+12 V</td><td>",join(" ",($rhkd->{'p12v'},'V<br>',$rhkd->{'p12i'},'A')),"</td></tr>\n";
	print "<tr><td>+12IP V</td><td>",join(" ",($rhkd->{'p12ipv'},'V<br>',$rhkd->{'p12ipi'},'A')),"</td></tr>\n";
	print "<tr><td>+24 V</td><td>",join(" ",($rhkd->{'batv'},'V<br>',$rhkd->{'p24i'},'A')),"</td></tr>\n";
	print "<tr><td>+PV V</td><td>",join(" ",($rhkd->{'ppvv'},'V<br>',$rhkd->{'ppvi'},'A')),"</td></tr>\n";
	print "<tr><td>-5 V</td><td>",join(" ",($rhkd->{'n5v'},'V<br>',$rhkd->{'n5i'},'A')),"</td></tr>\n";
	print "<tr><td>-12 V</td><td>",join(" ",($rhkd->{'n12v'},'V<br>',$rhkd->{'n12i'},'A')),"</td></tr>\n";
	print "<tr><td>GPS V</td><td>",$rhkd->{'gpsv'}," V</td></tr>\n";
	print "<tr><td>RFCM V</td><td>",$rhkd->{'rfcmv'}," V</td></tr>\n";
	print "<tr><td>Bat. I</td><td>",$rhkd->{'bati'}," A</td></tr>\n";
	print "</table></td><td valign=top><table border=1>\n";
	print "<tr><td>T 1-5</td><td>",join(" : ",($rhkd->{'t1'},$rhkd->{'t2'},$rhkd->{'t3'},$rhkd->{'t4'},$rhkd->{'t5'}))," C</td></tr>\n";
	print "<tr><td>T 6-10</td><td>",join(" : ",($rhkd->{'t6'},$rhkd->{'t7'},$rhkd->{'t8'},$rhkd->{'t9'},$rhkd->{'t10'}))," C</td></tr>\n";
	print "<tr><td>T 11-15</td><td>",join(" : ",($rhkd->{'t11'},$rhkd->{'t12'},$rhkd->{'t13'},$rhkd->{'t14'},$rhkd->{'t15'}))," C</td></tr>\n";
	print "<tr><td>T 16-20</td><td>",join(" : ",($rhkd->{'t16'},$rhkd->{'t17'},$rhkd->{'t18'},$rhkd->{'t19'},$rhkd->{'t20'}))," C</td></tr>\n";
	print "<tr><td>T 21-25</td><td>",join(" : ",($rhkd->{'t21'},$rhkd->{'t22'},$rhkd->{'t23'},$rhkd->{'t24'},$rhkd->{'t25'}))," C</td></tr>\n";
	print "<tr><td>T 26-30</td><td>",join(" : ",($rhkd->{'t26'},$rhkd->{'t27'},$rhkd->{'t28'},$rhkd->{'t29'},$rhkd->{'t30'}))," C</td></tr>\n";
	print "<tr><td>T 31-35</td><td>",join(" : ",($rhkd->{'t31'},$rhkd->{'t32'},$rhkd->{'t33'},$rhkd->{'t34'},$rhkd->{'t35'}))," C</td></tr>\n";
	print "<tr><td>T 36-40</td><td>",join(" : ",($rhkd->{'t36'},$rhkd->{'t37'},$rhkd->{'t38'},$rhkd->{'t39'},$rhkd->{'t40'}))," C</td></tr>\n";
	print "<tr><td>T 41-45</td><td>",join(" : ",($rhkd->{'t41'},$rhkd->{'t42'},$rhkd->{'t43'},$rhkd->{'t44'},$rhkd->{'t45'}))," C</td></tr>\n";
	print "<tr><td>T 46-50</td><td>",join(" : ",($rhkd->{'t46'},$rhkd->{'t47'},$rhkd->{'t48'},$rhkd->{'t49'},$rhkd->{'t50'}))," C</td></tr>\n";
	print "</table></td></tr>\n";
      }
      $sthhkd->finish;
    }

    print "<td colspan=2><b>RF event</b></td></tr>\n";
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
    print "</td><td valign=top colspan=2>\n";

    # Waveforms
    my $sthwv=$dbh->prepare("SELECT * FROM wvem WHERE evnum=$evnum");
    print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sthwv));
    print "Cannot execute $DBI::errstr<br>\n" if (!$sthwv->execute);
    if(my $rwv=$sthwv->fetchrow_hashref){
      print "<table>\n";
      my $i=0;
      my @chlist=("h1","v1","h2","v2","veto_h1","veto_v1","veto_h2","veto_v2");
      foreach my $ch (@chlist){
	$i++;
	my $val=substr $rwv->{$ch},1,-1; # Strips {}
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
	my $tmpfile="tmp/$ch-$$.jpg";
	open(IMG, ">$tmpfile") or die $!;
	binmode IMG;
	print IMG $graph->plot(\@data)->jpeg;
	close(IMG);
	print "<tr>" if ($i%2);
	print "<td><img src=\"$tmpfile\"></td>\n";
	print "</tr>" unless ($i%2);
	push @delete_list,$tmpfile;
      }
      print "</table>\n";
    }else{
      print "<b>No waveform</b>";
    }
    $sthwv->finish;
    print "</td></tr>\n";
    print "</table>\n";
  }else{
    print "Event not found!\n";
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
