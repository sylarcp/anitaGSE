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
Refresh: $refrate;top.cgi

<html>
<body>
<pre>
HEAD

print `top -b -n 1 -c| sed -n '1,/^\$/p;/postgres\\|perl\\|unpackd/p'`;

print  <<TAIL;
</pre>
</body>
</html>
TAIL
