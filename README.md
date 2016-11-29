# anitaGSE
##Install:
```
vi unpack/utilityFunctions.h # add #include <cstring>
declare -x CPPFLAGS=-I$ANITA_FLIGHT_SOFT_DIR/common
declare -x FLIGHTSOFT_LIBDIR=$ANITA_FLIGHT_SOFT_DIR/lib
cd ~/anitaGSE
./configure
cd wvreader
vi Makefile
remove -all static from LDFLAGS
make
```


##Run:
```
change the tcp/rules,
vi gseconf
change the fanout IP depends on your need.
./gseconf
./gsecontrol run 5
#terminate
./gsecontrol terminate
```

##Rotate database automatically: (Thanks Shige)
In crontab.rotate file:
Change the hour number in crontab.rotate according to your current zone.
Change the folder to where your anitaGSE is.
run the crontab by:
```
crontab crontab.rotate
```

##set up the GSE monitor webpage
The  monitor webpage is already in your local IP + '/anita'.
Besure to open port 80 to other people.
In terminal you can use 
```
links 127.0.0.1/anita
```
to check the website locally.

If you meet some .cgi display issues, fix as follow:
```
as root, 
vi /etc/https/conf/httpd.conf
Add or change to:
<Directory "var/www/html">
   Options +ExecCGI
   AddHandler cgi-script .cgi .pl
</Directory>

```

