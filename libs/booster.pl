#!/usr/bin/perl
# ===========================================================================
# OOMon - Objected Oriented Monitor Bot
# Copyright (C) 2004  Timothy L. Jensen
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# ===========================================================================

# $Id$

# This is a simple little script that searches through all the boost files
# and looks for missing #included files.  It then tries to copy them from
# a full boost source directory.  It may be necessary to run this script
# multiple times.


my $BOOST_DIR = "../../../boost_1_31_0/$filename";


my @includes;
foreach $file (getfiles("./boost"))
{
	push(@includes, getincludes($file));
}


foreach $filename (sort { $a cmp $b } uniq(@includes) )
{
	#print "$filename\n";
	if (!-f "$filename")
	{
		print "MISSING: $filename\n";
		if ($filename =~ /^(\S+)\/([^\/]+)$/)
		{
			my $dir = $1;
			my $file = $2;

			system("mkdir -p $dir");

			system("cp $BOOST_DIR $dir/");
		}
	}
}

exit(0);


sub uniq(@)
{
	my %hash = map { ($_,0) } @_;
	keys %hash;
}


sub getfiles($)
{
	my ($dirname) = @_;
	my @files;
	my @dirs;

	if (opendir(DIR, "$dirname"))
	{
		while (my $filename = readdir(DIR))
		{
			if (-f "$dirname/$filename")
			{
				#print "FILE: $dirname/$filename\n";
				push(@files, "$dirname/$filename");
			}
			elsif ( !/^\./ && -d "$dirname/$filename")
			{
				#print "DIR: $dirname/$filename\n";
				push(@dirs, "$dirname/$filename");
			}
		}
		closedir(DIR);
	}
	foreach (@dirs)
	{
		push(@files, getfiles($_));
	}
	@files;
}


sub getincludes($)
{
	my ($filename) = @_;
	my @result;

	if (open(FILE, "< $filename "))
	{
		while (<FILE>)
		{
			chomp;
			if (/^\#\s*include\s+[\<\"](boost\/\S+\.hpp)[\>\"].*$/)
			{
				#print "INCLUDE: $1\n";
				push(@result, $1);
			}
		}
		close(FILE);
	}
	@result;
}

