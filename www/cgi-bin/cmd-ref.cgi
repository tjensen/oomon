#!/usr/bin/perl

use CGI qw(:standard escape escapeHTML);
use POSIX qw(strftime);

$invStyle =<<END;

a { color: white; }

a:hover { 
        color: white; 
        background: rgb(102, 0, 0); 
        text-decoration: none;
}

body { 
        background-color: rgb(0, 0, 0); 
	font-family: "Trebuchet MS", Verdana, Arial, sans-serif; 
}

h2 { 
	color: white; 
	background: rgb(51, 51, 51); 
	font: bold 17px "Trebuchet MS", Verdana, Arial, sans-serif; 
	margin: 10px 0px 0px 10px; 
	padding: 5px 15px 5px 5px;
       	width: 90%;
        text-align: left;
}

h1 {
	font: bold 40px "Trebuchet MS", Verdana, Arial, sans-serif; 
        letter-spacing: -2px;
}

code, pre {
        font: normal 12px monospace;
}

li {
        list-style-type: none;
}

.section { 
	color: white; 
	background: rgb(10, 10, 10);
	font: normal 14px Verdana, Tahoma, Arial, sans-serif; 
	margin: 0px 0px 0px 10px; 
	padding: 5px 10px 5px 10px; 
	width: 90%; 
	text-align: justify;
        line-height: 1.4;
}

#techfooter {
	background-color: rgb(51, 51, 51); 
        font: 12px "Trebuchet MS", Verdana, Arial, sans-serif;
        text-align: center;
        margin: 10px;
        padding: 0px 10px 0px 10px;
        color: white;
        width: 100%;
}

#techfooter p {
        padding: 5px;
}

END

$topic = lc(url_param('topic')) || '';
$version = url_param('version') || '2.3.1';

$cgifile = 'cmd-ref';
$helpfile = "oomon-$version.help";

print	header;
print	start_html(-title=>"OOMon Command Reference: " . $topic, -style=>{-code=>$invStyle});

print   "\n\n";

print   "<div class=\"section\">\n";

print   "<h1><a href=\"http://oomon.sourceforge.net\">OOMon</a> Command Reference</h1>\n";

print	startform('get', $cgifile);
print	"<p>Search: " . textfield(-name=>'topic',
		-default=>$topic,
		-size=>32,
		-maxlength=>80);
print	"\&nbsp;" . submit(-value=>'Help');
print	"</p>\n";
print	"Version: " . popup_menu(-name=>'version',
		-values=>['2.3.1', '2.3', '2.2', '2.1', '2.0', '1.2.1', '1.2'],
		-default=>$version);
print	endform;

print	"<p><a href=\"$cgifile?version=$version\">^-- Command Index</a></p>\n";
print	"<p><a href=\"/?p=home\"><-- Home</a></p>\n";

print   "</div>";

if ($topic)
{
	if (open(HELP, "< $helpfile "))
	{
		my @topics = ();
		my @syntax = ();
		my @description = ();
		my @example = ();
		my $flags = "";
		my @link = ();
		my @subtopic = ();
		my $inTopic = 0;

		while (<HELP>)
		{
			chop;
			if (/^\.(.)\.(.*)$/)
			{
				my $type = $1;
				my $value = $2;

				next if ($inTopic == 0);

				if ($type eq "s")
				{
					@syntax = (@syntax, $value);
				}
				elsif ($type eq "d")
				{
					@description = (@description, $value);
				}
				elsif ($type eq "e")
				{
					@example = (@example, $value);
				}
				elsif ($type eq "f")
				{
					$flags = $value;
				}
				elsif ($type eq "l")
				{
					@link = (@link, $value);
				}
				elsif ($type eq "t")
				{
					@subtopic = (@subtopic, $value);
				}
			}
			elsif (!/^\#/)
			{
				my @foo = split(/\./);

				$inTopic = 0;
				foreach $tmp (@foo)
				{
					if (lc($tmp) eq lc($topic))
					{
						$inTopic = 1;
						@topics = @foo;
						break;
					}
				}
			}
		}
		close(HELP);

		if ($#topics >= 0)
		{
			if ($#syntax >= 0)
			{
				print "<h2>Syntax:</h2>\n";
				print "<div class=\"section\"><p>\n";
				foreach $tmp (@syntax)
				{
					print "<code>." . escapeHTML($tmp) . "</code><br />\n";
				}
				print "</p></div>\n";
			}

			if ($#description >= 0)
			{
				print "<h2>Description:</h2>\n";
				print "<div class=\"section\"><p>\n";
				foreach $tmp (@description)
				{
					if ($tmp eq "")
					{
						print "</p><p>\n";
					}
					else
					{
						print escapeHTML($tmp) . "\n";
					}
				}
				print "</p></div>\n";
			}

			if ($#example >= 0)
			{
				print "<h2>Example:</h2>\n";
				print "<div class=\"section\"><pre>";
				foreach $tmp (@example)
				{
					print escapeHTML($tmp) . "\n";
				}
				print "</pre></div>\n";
			}

			print "<h2>Required Flags:</h2>\n";
			print "<div class=\"section\"><p>\n";
			if ($flags eq "")
			{
				print "<b>None</b>\n";
			}
			else
			{
				print "<b>C</b> - Chanop<br />\n" if ($flags =~ /c/i);
				print "<b>D</b> - Dline<br />\n" if ($flags =~ /d/i);
				print "<b>G</b> - Gline<br />\n" if ($flags =~ /g/i);
				print "<b>K</b> - Kline<br />\n" if ($flags =~ /k/i);
				print "<b>M</b> - Master<br />\n" if ($flags =~ /m/i);
				print "<b>O</b> - Oper<br />\n" if ($flags =~ /o/i);
				print "<b>R</b> - Remote<br />\n" if ($flags =~ /r/i);
				print "<b>W</b> - Wallops<br />\n" if ($flags =~ /w/i);
			}
			print "</p></div>\n";

			if ($#link >= 0)
			{
				print "<h2>See Also:</h2>\n";
				print "<div class=\"section\"><p>\n";
				foreach $tmp (@link)
				{
					print "<a href=\"$cgifile?topic=$tmp\&amp;version=$version\">$tmp</a><br />\n";
				}
				print "</p></div>\n";
			}

			if ($#subtopic >= 0)
			{
				print "<h2>Sub-Topics:</h2>\n";
				print "<div class=\"section\"><p><table width=100% border=0><tr>\n";
				foreach $tmp (sort @subtopic)
				{
					$column++;
					print "<td><a href=\"$cgifile?topic=$topic\%20$tmp\&amp;version=$version\">$tmp</a>\&nbsp;</td>\n";
					if ($column == 5)
					{
						print "</tr><tr>\n";
						$column = 0;
					}
				}
				print "</tr></table></p></div>\n";
			}
		}
		else
		{
			print "<p>No help available for: $topic</p>\n";
		}
	}
	else
	{
		print "<p>Unable to read help file!</p>\n";
	}
}
else
{
	if (open(HELP, "< $helpfile "))
	{
		my @topics = ();
		my $column = 0;

		while (<HELP>)
		{
			chop;
			if (/^[^\#\.]/)
			{
				my @items = split(/\./);

				foreach $item (@items)
				{
					if ($item !~ / /)
					{
						@topics = (@topics, $item);
					}
				}
			}
		}
		close(HELP);

		print "<h2>Command Index:</h2>\n";
		print "<div class=\"section\"><table width=\"100%\" border=\"0\"><tr>\n";
		foreach $topic (sort @topics)
		{
			$column++;
			print "<td><a href=\"$cgifile?topic=$topic\&amp;version=$version\">$topic</a>\&nbsp;</td>\n";
			if ($column == 5)
			{
				print "</tr><tr>\n";
				$column = 0;
			}
		}
		print "</tr></table></div>\n";
	}
	else
	{
		print "<p>Unable to read help file!</p>\n";
	}
}

#print hr;

#print "<a href=\"index.html\">Back to Main Page</a>\n";

print hr;

#print "<a href=\"http://sourceforge.net\"><img src=\"http://sourceforge.net/sflogo.php?group_id=94236&amp;type=2\" width=\"125\" height=\"37\" border=\"0\" alt=\"SourceForge.net Logo\" /></a>\n";
print "<a href=\"http://sourceforge.net/\"><img style=\"border: 0\"
            src=\" http://sourceforge.net/sflogo.php?group_id=94236&amp;type=2\"
            width=\"125\" height=\"37\" alt=\"SourceForge.net Logo\" /></a>";
#print hr;

#print "<small><em>This site Copyright \&copy; 1999-";
#print strftime("%Y", localtime);
#print " Timothy L. Jensen</em><br>\n";
#print '$Id$</small>' . "\n";

print end_html;

exit;

