#!/bin/sh
PORT=7740
HANDLER=/home/anita/anitaGse/tcp/receive_data.pl
exec 2>&1
exec tcpserver -v -R -x /home/anita/anitaGse/tcp/rules.cdb 0 $PORT $HANDLER
