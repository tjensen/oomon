#!/usr/bin/perl

use CGI qw(:standard escape escapeHTML);
use POSIX qw(strftime);

$invStyle =<<END;
	FONT.inv {
		color: black;
		background: white;
	}
END

$topic = lc(url_param('topic')) || '';
$version = url_param('version') || '2.3.1';

$cgifile = 'help.cgi';
$helpfile = "oomon-$version.help";

print	header;
print	start_html(-title=>"OOMon Help: " . $topic, -TEXT=>'#FFFFFF',
	-BGCOLOR=>'#000030', -LINK=>'#FF5050', -ALINK=>'#00FF00',
	-VLINK=>'FF0FF00', -style=>{-code=>$invStyle});
print "\n";

print "<FONT SIZE=+7>OOMon</FONT><P>\n";

print	startform('get', $cgifile);
print	"Help: " . textfield(-name=>'topic',
		-default=>$topic,
		-size=>32,
		-maxlength=>80);
print	"\&nbsp;" . submit(-value=>'Help');
print	"<BR>\n";
print	"Version: " . popup_menu(-name=>'version',
		-values=>['2.3.1', '2.3', '2.2', '2.1', '2.0', '1.2.1', '1.2'],
		-default=>$version);
print	endform;

print	"<P><A HREF=\"$cgifile?version=$version\">Help Index</A>\n";

print hr;

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
				print "<H1>Syntax:</H1>\n";
				print "<UL>\n";
				foreach $tmp (@syntax)
				{
					print "<TT>." . escapeHTML($tmp) . "</TT><BR>\n";
				}
				print "</UL>\n";
			}

			if ($#description >= 0)
			{
				print "<H1>Description:</H1>\n";
				print "<UL>\n";
				foreach $tmp (@description)
				{
					if ($tmp eq "")
					{
						print "<P>\n";
					}
					else
					{
						print escapeHTML($tmp) . "\n";
					}
				}
				print "</UL>\n";
			}

			if ($#example >= 0)
			{
				print "<H1>Example:</H1>\n";
				print "<UL><PRE>";
				foreach $tmp (@example)
				{
					print escapeHTML($tmp) . "\n";
				}
				print "</PRE></UL>\n";
			}

			print "<H1>Required Flags:</H1>\n";
			print "<UL>\n";
			if ($flags eq "")
			{
				print "<B>None</B><BR>\n";
			}
			else
			{
				print "<B>C</B> - Chanop<BR>\n" if ($flags =~ /c/i);
				print "<B>D</B> - Dline<BR>\n" if ($flags =~ /d/i);
				print "<B>G</B> - Gline<BR>\n" if ($flags =~ /g/i);
				print "<B>K</B> - Kline<BR>\n" if ($flags =~ /k/i);
				print "<B>M</B> - Master<BR>\n" if ($flags =~ /m/i);
				print "<B>O</B> - Oper<BR>\n" if ($flags =~ /o/i);
				print "<B>R</B> - Remote<BR>\n" if ($flags =~ /r/i);
				print "<B>W</B> - Wallops<BR>\n" if ($flags =~ /w/i);
			}
			print "</UL>\n";

			if ($#link >= 0)
			{
				print "<H1>See Also:</H1>\n";
				print "<UL>\n";
				foreach $tmp (@link)
				{
					print "<A HREF=\"$cgifile?topic=$tmp\&version=$version\">$tmp</A><BR>\n";
				}
				print "</UL>\n";
			}

			if ($#subtopic >= 0)
			{
				print "<H1>Sub-Topics:</H1>\n";
				print "<UL><TABLE WIDTH=100% BORDER=0><TR>\n";
				foreach $tmp (sort @subtopic)
				{
					$column++;
					print "<TD><A HREF=\"$cgifile?topic=$topic\%20$tmp\&version=$version\">$tmp</A>\&nbsp;</TD>\n";
					if ($column == 5)
					{
						print "</TR><TR>\n";
						$column = 0;
					}
				}
				print "</TR></TABLE></UL>\n";
			}
		}
		else
		{
			print "No help available for: $topic\n";
		}
	}
	else
	{
		print "Unable to read help file!\n";
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

		print "<H1>Help Index:</H1>\n";
		print "<UL><TABLE WIDTH=100% BORDER=0><TR>\n";
		foreach $topic (sort @topics)
		{
			$column++;
			print "<TD><A HREF=\"$cgifile?topic=$topic\&version=$version\">$topic</A>\&nbsp;</TD>\n";
			if ($column == 5)
			{
				print "</TR><TR>\n";
				$column = 0;
			}
		}
		print "</TR></TABLE></UL>\n";
	}
	else
	{
		print "Unable to read help file!\n";
	}
}

print hr;

print "<A HREF=\"index.html\">Back to Main Page</A>\n";

print hr;

print "<a href=\"http://sourceforge.net\"><img src=\"http://sourceforge.net/sflogo.php?group_id=94236&amp;type=2\" width=\"125\" height=\"37\" border=\"0\" alt=\"SourceForge.net Logo\" /></a>\n";

print hr;

print "<SMALL><EM>This site Copyright \&copy; 1999-";
print strftime("%Y", localtime);
print " Timothy L. Jensen</EM><BR>\n";
print '$Id$</SMALL>' . "\n";

print end_html;

exit;

