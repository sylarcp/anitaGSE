#!/usr/bin/perl

$tcpclient="/home/ped/anisoft/ucspi-tcp-0.88-rh9/tcpclient";
$ip="127.0.0.1";
$port=7740;
$reprate=1;

$target_file="test.tgz";

while(1){
  unless(fork){   # Fork this out, so that we don't have to wait
    system("$tcpclient -v $ip $port sh -c \"cat $target_file 1>&7\" 1>&2");
    exit;
  }
  sleep $reprate;
  last;
}


