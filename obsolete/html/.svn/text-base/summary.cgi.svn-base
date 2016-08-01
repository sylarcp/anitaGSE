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
use GD::Graph::bars;
use POSIX qw(ceil floor);
use Time::Local;
require 'anitagse.pi';

# Get calling parameters
my $cgi = new CGI;
my $last = $cgi->param("last");
my $start = $cgi->param("start");
my $end = $cgi->param("end");

# Keep track of temporary files
my @delete_list=();

# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
my $refrate=300;  # Refresh rate in seconds
print <<HEAD;
Content-Type: text/html
Refresh: $refrate;summary.cgi?last=$last&start=$start&end=$end

<html>
<body>
HEAD

if ($last>0){
  $end=time;
#  $end=1124313440;
  $start=$end-$last;
}elsif($last==-1){
  $end=time;
#  $end=1124313440;
  $start=1;
}elsif($last==-2){
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
    print "<td><form action=summary.cgi method=\"post\">\n";
    print "<input type=\"hidden\" name=\"last\" value=\"-3\">\n";
    print "<input type=\"hidden\" name=\"end\" value=\"$end\">\n";
    my $new_start=$start+$dt;
    print "<input type=\"hidden\" name=\"start\" value=\"$new_start\">\n";
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
    print "<td><form action=summary.cgi method=\"post\">\n";
    print "<input type=\"hidden\" name=\"last\" value=\"-3\">\n";
    my $new_end=$end+$dt;
    print "<input type=\"hidden\" name=\"end\" value=\"$new_end\">\n";
    print "<input type=\"hidden\" name=\"start\" value=\"$start\">\n";
    print "<input type=\"submit\" value=\"$lbl\">\n";
    print "</form></td>\n";
    print "</tr>" if ($i%3==0);
  }
  print "</tr></table></td></tr></table>\n";

  # Make connection to DB and retrieve latest info
  my $dbh=DBI->connect("DBI:Pg:dbname=emflight");
  if($dbh){
    my $sth;
    my $gd;
    print "<table cellpadding=10%>\n";
    # Count raw packets
    my @hvarlist=qw(hkd:hkd_raw
		    gpsP:gpsd_pat
		    gpsS:gpsd_sat
		    hd:hd
		    cmd:cmd
		   );
    my %pckcnt;
    foreach my $varitem (@hvarlist){
      my ($lbl,$tbl)=split /:/,$varitem;
      $sth = $dbh->prepare("SELECT COUNT(time) FROM $tbl WHERE (time>=$start AND time<=$end)");
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
      $pckcnt{$lbl}=$sth->fetchrow;
    }
    # Raw waveform packets don't have time encoded, so we use this special querry
    $sth = $dbh->prepare("SELECT COUNT(pck) FROM wv_raw WHERE evnum IN (SELECT evnum FROM hd WHERE time>=$start AND time<=$end)");
    print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
    print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
    $pckcnt{'wv/16'}=$sth->fetchrow/16;

    my $startstr=timestr(gmtime($start));
    my @pckdata=([keys %pckcnt],[values %pckcnt]);
    my $hgraph = GD::Graph::bars->new(250, 200);
    $hgraph->set(x_label           => 'type',
		  y_label           => 'packets [N]',
		  title             => "$startstr",
		  y_min_value       => 0,
#		  y_tick_number     => 10,
#		  y_label_skip      => 1,
		  r_margin          => 10
		 ) or print "$hgraph->error<br>\n";
    $gd=$hgraph->plot(\@pckdata);
    my $tmpfile="tmp/pckcnt-$$.jpg";
    open(IMG, ">$tmpfile") or print "$!<br>\n";
    binmode IMG;
    print IMG $gd->jpeg or print "$hgraph->error<br>\n";
    close(IMG);
    print "<tr>\n";
    print  "<td><img src=\"$tmpfile\"></td>\n";
    push @delete_list,$tmpfile;
    $sth->finish;

    # Count event priorities
    my @prcnt;
    foreach my $p (0..9){
      $sth = $dbh->prepare("SELECT COUNT(nbuf) FROM hd WHERE (time>=$start AND time<=$end AND priority=$p)");
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
      $prcnt[$p]=$sth->fetchrow;
    }
    my $startstr=timestr(gmtime($start));
    my @prdata=([0..9],[@prcnt]);
    $hgraph = GD::Graph::bars->new(250, 200);
    $hgraph->set(x_label           => 'event priority',
		  y_label           => 'packets [N]',
		  title             => "$startstr",
		  y_min_value       => 0,
#		  y_tick_number     => 10,
#		  y_label_skip      => 1,
		  r_margin          => 10
		 ) or print "$hgraph->error<br>\n";
    $gd=$hgraph->plot(\@prdata);
    my $tmpfile="tmp/prcnt-$$.jpg";
    open(IMG, ">$tmpfile") or print "$!<br>\n";
    binmode IMG;
    print IMG $gd->jpeg or print "$hgraph->error<br>\n";
    close(IMG);
    print  "<td><img src=\"$tmpfile\"></td>\n";
    push @delete_list,$tmpfile;
    $sth->finish;

    # Count trigger types
    $sth = $dbh->prepare("SELECT trigtype FROM hd WHERE (time>=$start AND time<=$end)");
    print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
    print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
    my %typecode=(1 => 'RF',2 => 'ADU5',4 => 'G12',8 => 'SFT');
    my %typecnt=(1 => 0,2 => 0,4 => 0,8 => 0);;
    while(my $trigtype=$sth->fetchrow){
      my $trig=($trigtype&0x0f);
      ++$typecnt{$trig} if defined $typecnt{$trig};
    }
    my @xval;
    my @yval;
    foreach my $k (sort keys%typecode ){
      push @xval,$typecode{$k};
      push @yval,$typecnt{$k};
    }
    my @typedata=([@xval],[@yval]);
    $hgraph = GD::Graph::bars->new(250, 200);
    $hgraph->set(x_label           => 'trigger type',
		  y_label           => 'triggers [N]',
		  title             => "$startstr",
		  y_min_value       => 0,
#		  y_tick_number     => 10,
#		  y_label_skip      => 1,
		  r_margin          => 10
		 ) or print "$hgraph->error<br>\n";
    $gd=$hgraph->plot(\@typedata);
    my $tmpfile="tmp/typecnt-$$.jpg";
    open(IMG, ">$tmpfile") or print "$!<br>\n";
    binmode IMG;
    print IMG $gd->jpeg or print "$hgraph->error<br>\n";
    close(IMG);
    print  "<td><img src=\"$tmpfile\"></td>\n";
    push @delete_list,$tmpfile;
    $sth->finish;
    print "</tr>\n";

    # Plots of time dependent variables
    my @varlist=qw(gpsd_pat:latitude:deg:.2f
		 gpsd_pat:longitude:deg:.2f
		 gpsd_pat:altitude:m:.1f
		 gpsd_pat:heading:deg:.2f
		 gpsd_pat:pitch,roll:deg:.2f
		 gpsd_pat:mrms,brms:m:.2f
		 gpsd_sat:numsats::.1f
		 hkd:acc1x,acc2x:g:.3f
		 hkd:acc1y,acc2y:g:.3f
		 hkd:acc1z,acc2z:g:.3f
		 hkd:acc1t,acc2t,sst:C:d
		 hkd:ssx,ssy::.3f
                 hkd:pressh:PSI:.3f
                 hkd:pressl:Torr:.3f
                 hkd:magx,magy,magz:g:.3f
                 hkd:sbst1,sbst2:C:.1f
                 hkd:p1_5v,p2_5v,p3_3v:V:.1f
		 hkd:n5v,p5v:V:.2f
                 hkd:n12v,p12v,p12ipv:V:.2f
		 hkd:ppvv,batv:V:.1f
		 hkd:gpsv,rfcmv:V:.2f
                 hkd:p1_5i,p2_5i,p3_3i:A:.3f
                 hkd:p5i,p12i,p12ipi:A:.3f
		 hkd:ppvi,p24i:A:.3f
		 hkd:bati:A:.3f
		 hkd:n5i,n12i:A:.3f
		 hkd:t1,t2,t3,t4,t5:C:.1f
		 hkd:t6,t7,t8,t9,t10:C:.1f
		 hkd:t11,t12,t13,t14,t15:C:.1f
		 hkd:t16,t17,t18,t19,t20:C:.1f
		 hkd:t26,t27,t28,t29,t30:C:.1f
		 hkd:t31,t32,t33,t34,t35:C:.1f
		 hkd:t36,t37,t38,t39,t40:C:.1f
		 hkd:t46,t47,t48,t49,t50:C:.1f
		  );
    my $n=0;
    foreach my $varitem (@varlist){
      my ($tbl,$var,$unit,$fmt)=split /:/,$varitem;
      $sth=$dbh->prepare("SELECT time,$var FROM $tbl WHERE (time>=$start AND time<=$end) ORDER BY time");;
      print "Cannot prepare $DBI::errstr<br>\n" if (!defined($sth));
      print "Cannot execute $DBI::errstr<br>\n" if (!$sth->execute);
      my ($data_ref,$reftime)=makedata($sth,"time",split(/,/,$var));
      $startstr=timestr(gmtime($reftime));
      my $graph = GD::Graph::lines->new(300, 200);
      my $maxx=(int($data_ref->[0][-1]/10)+1)*10;
      $graph->set(x_label           => 't [min]',
		  y_label           => "$var [$unit]",
		  title             => "t=0 @ $startstr",
#		  y_max_value       => $maxy,
#		  y_min_value       => $miny,
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
      #print "$var ",length(@{$data_ref}),"<br>\n";
      #print join("-",split(/,/,$var)),"<br>\n";
      $graph->set_legend(split(/,/,$var));
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
      $tmpfile="tmp/$var-$$.jpg";
      open(IMG, ">$tmpfile") or print "$!<br>\n";
      binmode IMG;
      print IMG $gd->jpeg or print "$graph->error<br>\n";
      close(IMG);
      print "<tr>\n " if ($n%3==0);
      print  "<td><img src=\"$tmpfile\"></td>\n";
      print "</tr>\n" if ($n%3==2);
      push @delete_list,$tmpfile;
      $sth->finish;
      $n++;
    }
    print "</table>\n";
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


