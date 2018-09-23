<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
     "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<?php /* $Id$ 
       *
       * This is part of an effort to reduce the amount of CVS keywords 
       * in the HTML source. */ ?>

<?php include "latest-rel.inc"; 
		/* This file contains the version number of the latest release. 
		 * Changing the number in latest-rel.inc will change the version
		 * number displayed in the sidebar, as well as the link to the 
		 * tarball. */ ?>

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head>
  <title>OOMon - The Object-Oriented IRC Server Connection Monitor Bot</title>
  <meta http-equiv="Content-Type" content= "text/html; charset=us-ascii" />
  <link rel="shortcut icon" href="/favicon.ico" />
<style type="text/css" media="screen">
/*<![CDATA[*/

/* CSS files included dynamically for portability. */

<?php 
      /* I figured including the CSS info via PHP would be more
       * beneficial to users, since they may be saving pages to
       * local drives and forgetting to save the CSS file to go
       * along with it. This also allows for the style to be in
       * a separate place for easy editing.  --avj */

	    if (ereg("(.*MSIE.*Windows.*)", $HTTP_USER_AGENT)) {
	       $has_ie = 1;
	       include "oomon-ie-win.css";
	    }
	    else {
	       include "reset.css";
	       include "oomon.css";
	    }
?>
/*]]>*/
</style>
</head>

<body>
  <div id="all">
    <h1 id="header"><a href="?p=home">OOMon: The OO IRC Monitor Bot</a></h1>

    <div id="menu">
      <ul>

        <li id="local-links">Local Links:
          <ul>
				<li><a href="?p=home"
            title="The latest OOMon information">Home</a>
            </li>

				<li><a href="?p=faq"
            title="Frequently Asked Questions">FAQ</a></li>

				<li><a href="?p=cvs-install"
            title="How to build OOMon from CVS sources">CVS Installation</a></li>

				<li><a href="?p=snap-install"
            title="How to build OOMon from a development snapshot">Snapshot Installation</a></li>

				<li><a href="?p=oomon-cf"
            title="The oomon.cf file, demystified">Configuration</a></li>

				<li><a href="/cgi-bin/cmd-ref.cgi"
            title="Command reference">Command Reference</a></li>
          </ul>
        </li>

        <li id="sf-links">SourceForge Links:
          <ul>
            <li><a href="http://sourceforge.net/projects/oomon/"
            title="Main OOMon project page">Project Page</a></li>

            <li><a href= "http://sourceforge.net/project/showfiles.php?group_id=94236"
            title="Browse the released files related to this project">File List</a></li>

            <li><a href= "http://sourceforge.net/cvs/?group_id=94236"
            title= "Information regarding anonymous CVS access">Anonymous CVS</a></li>

            <li><a href= "http://oomon.cvs.sourceforge.net/oomon/oomon/"
            title= "Browse the CVS repository online via ViewVC">Browse CVS</a></li>

            <li><a href= "http://sourceforge.net/mail/?group_id=94236"
            title= "Join an oomon-related mailing list">Mailing Lists</a></li>

            <li><a href= "http://sourceforge.net/tracker/?group_id=94236&amp;atid=607186"
            title="Enter new or track existing OOMon bugs">Bug Tracker</a></li>

            <li><a href= "http://sourceforge.net/tracker/?group_id=94236&amp;atid=607189"
            title="Request a new feature for OOMon">Feature Request</a></li>

            <li><a href= "http://sourceforge.net/project/memberlist.php?group_id=94236"
            title="Contact OOMon contributors">Contact</a></li>
          </ul>
        </li>

        <li id="latest-release">Latest Release: 
          <ul>
            <li><a href="http://prdownloads.sourceforge.net/oomon/oomon-<?php echo $lrel; ?>.tar.gz?download"
            title="Latest Release">oomon-<?php echo $lrel; ?>.tar.gz</a></li>
          </ul>
        </li>

        <li id="latest-snap">Latest Snapshot:
          <ul>
            <li><a href="ftp://grind.realfx.net/pub/irc/oomon/oomon-latest_SNAP.tar.gz"
            title="Latest Snapshot">oomon.tar.gz</a></li>
          </ul>
        </li>

        <li id="sf-include">Sponsored By:
          <ul>
            <li><a href="http://sourceforge.net/"><img style= "border: 0"
            src= "http://sourceforge.net/sflogo.php?group_id=94236&amp;type=2"
            width="125" height="37" alt="SourceForge.net Logo" /></a></li>
          </ul>
        </li>

      </ul>
    </div><!-- end div#menu -->

    <div id="content">

<?php   /* Any time new pages are added to the site,
           they must be added to $page_array.  --avj */

		$page_array = array('home','faq','snap-install','cvs-install','oomon-cf'); 

		$page = $_GET['p'];  // allow ?p=foo instead of ?page=foo
		if(!$page)
			$page = 'home';   // if $page isn't specified, go home
		if(!in_array($page,$page_array)) 
			$page = 'home';   // if the requested page isn't in the array, go home

		include "$page.html";
?>

      <div id="techfooter">
        <p>This document is <a href=
        "http://validator.w3.org/check?uri=http://<?php echo "$HTTP_HOST" . "$REQUEST_URI"; ?>"
        title= "Validate this document's XHTML">XHTML 1.0 Strict</a> and <a href=
        "http://jigsaw.w3.org/css-validator/validator?uri=http://<?php echo "$HTTP_HOST" . "$REQUEST_URI"; ?>"
        title= "Validate this document's CSS">CSS 3</a> compliant. 
        <i>Site issues/problems? Contact &lt;avj&gt; [at] &lt;users.sf.net&gt;</i></p>
      </div><!-- end div#techfooter -->
    </div><!-- end div#content -->
  </div><!-- end div#all -->
</body>
</html>
