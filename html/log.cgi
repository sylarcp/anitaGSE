#!/usr/bin/perl
# -*- perl -*-

use strict;

use CGI;

# Get calling parameters
my $cgi = new CGI;
my $type = $cgi->param("type");

# Buffer output until entire page is ready
$|=1;
# Print HTML header information, title, etc.
my $refrate=5;  # Refresh rate in seconds
print <<HEAD;
Content-Type: text/html
Refresh: $refrate;log.cgi?type=$type

<html>
<body>
<pre>
HEAD

print `cat /tmp/anita_monitor_${type}.txt`;

print  <<TAIL;
</pre>
</body>
</html>
TAIL
