#!/usr/bin/perl
########################################################################
#
# LICENSE
#
# Copyright (C) 2005 Felix Suwald
#
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
# USA.
#
########################################################################
#
# CODE
#
# ripit.pl - Rips CD Audio and encodes files in the following steps:
#	     1) Query CDDB data for Album/Artist/Track info
#	     2) Rip the audio files from the CD
#	     3) Encode the wav files
#	     4) ID3 tag the encoded files
#	     5) Optional: Create a playlist (M3U) file.
#	     6) Optional: Prepares and sends a CDDB submission.
#
# Version 3.4 - August 5th 2005 - Felix Suwald and Max Kaesbauer
# Version 2.5 - November 13th 2004 - Felix Suwald
# Version 2.2 - October 18th 2003 - Mads Martin Joergensen
# Version 1.0 - February 16th 1999 - Simon Quinn
#
#
########################################################################
#
# User configurable variables:
# Keep these values and save your own settings
# in the config file with option --save!
#

use POSIX;
use Locale::gettext;

$ENV{PATH}="/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/opt/bin:/usr/i686-pc-linux-gnu/gcc-bin/3.3:/usr/X11R6/bin";

# !! CHANGE THIS TO YOUR ENV !!
$svdrpcmd       = "/usr/sbin/svdrpsend.sh"; # Full path of svdrpsend.pl command.
$cddevice	= "/dev/dvd";           # Path of audio CD device.

$output	        = "";		# Where the sound should go to.
$ripopt	        = "";		# Options for audio CD ripper.
$ripper	        = 1;		# 0 - dagrab, 1 - cdparanoia,
				# 2 - cdda2wav, 3 - tosha, 4 - cdd.
@coder		= (0);		# 0 - Lame, 1 - OggVorbis, 2 - Flac,
				# comma seperated list.
$bitrate	= 128;		# Bitrate for lame, if --vbrmode used,
				# bitrate is equal to the -b option.
$maxrate	= 320;		# Bitrate for lame using --vbrmode,
				# maxrate is equal to the -B option.
@quality	= (2,3,5,100);# Quality for lame in vbr mode (0-9),
				# best quality = 0, quality for ogg
				# (1-10) best = 10; or compression
				# level for Flac (0-8), lowest = 0.
$qualame	= 2;		#
$qualogg	= 3;		#
$quaflac	= 5;		#
$quafaac	= 100;		#
$lameopt	= "--vbr-new -V 2 -q 2 -b 128 -B 320 -p --preset insane";		#
$oggopt	= "";		#
$flacopt	= "";		#
$faacopt	= "";		#
$lcd		= 0;		# Use lcdproc (1 yes, 0 no).
$chars		= 0;		# Use special chars in files (1 yes, 0 no).
$quiet		= 1;		# Be quiet (no output) (1 yes, 0 no).
$commentag	= "";		# Comment ID3 tag.
$genre		= "";		# Genre of Audio CD for ID3 tag.
$year		= "";		# Year of Audio CD for ID3 tag.
$utftag	= 1;		# If zero decode utf tags to ISO8895-1.
$eject		= 0;		# Eject the CD when done (1 yes, 0 no).
$halt		= 0;		# Shutdown machine when finished
				# (1 yes, 0 no).
$nice		= 5;		# Set nice.
$savenew	= 0;		# Saved passed parameters to new config
				# file (1 yes, 0 no).
$savepara	= 0;		# Save parameters passed in config file
				# (1 yes, 0 no).
$config	= 0;		# Use config file to read paramaters
				# (1 yes, 0 no).
$submission	= 1;		# Send CDDB submission
				# (1 yes, 0 no).
$parano	= 0;		# Disable paranoia mode in cdparanoia
				# (1 yes, 0 no).
$playlist	= 0;		# Do create the m3u playlist file
				# (1 yes, 0 no).
$interaction	= 1;		# If 0 do not ask anything, take the 1st
				# CDDB entry found or use default names!
				# (1 yes, 0 no).
$underscore	= 0;		# Use _ instead of spaces in filenames
				# (1 yes, 0 no).
$lowercase	= 0;		# Lowercase filenames
				# (1 yes, 0 no).
$archive	= 0;		# Save CDDB files in ~/.cddb dir
				# (1 yes, 0 no).
$mirror	= "freedb";	# The host (a freedb mirror) that
				# shall be used instead of freedb.
$transfer	= "cddb";	# Transfer mode, cddb or http, will set
				# default port to 8880 or 80 (for http).
$vbrmode	= "new";		# Variable bitrate, only used with lame,
				# (new or old), see lame-manpage.
$proto		= 6;		# CDDB protocol level for CDDB query.
$proxy		= "";		# Set proxy.
$CDDB_HOST	= "freedb.org"; # Set cddb host
$mailad	= "";		# Users return e-mail address.
@sshlist	= ();		# List of remote machines.
$scp		= 0;		# Use scp to access files (1 yes, 0 no).
$local		= 1;		# Encode on locale machine (1 yes, 0 no).
$wav		= 0;		# Keep the wav files (1 yes, 0 no).
$encode	= 1;		# Encode the wav files (1 yes, 0 no).
$rip		= 1;		# Rip the CD files (1 yes, 0 no).
$multi		= 0;		# Not yet official. Do NOT use!
$field		= 0;		# Not yet supported by CDDB_get.pm.
#
#
# Directory and track template variables:
# Contains the format the track names will be written as.
# The '" and "' are important and must surround the template.
# Example variables to use are: $tracknum, $album, $artist, $genre,
# $trackname or $year.
# E.g. example setting of $tracktemplate produces a trackname of
# "07 The Game - Swandive" .
# $tracktemplate  = '"$tracknum $trackname - $artist"';
#
$dirtemplate = '"$artist - $album"';
$tracktemplate  = '"$tracknum $trackname"';
#
#
# lcdproc settings:
#
$lcdhost	= "localhost";
$lcdport	= "13666";
$lcdline1	= "   [initializing]   ";
$lcdline2	= " ripit-lcd-module     ";
$lcdline3	= "          2005 ";
$lcdoline1	= "";
$lcdoline2	= "";
$lcdoline3	= "";
$lcdproc;
$lcdtrackno	= 0;
#
$showosdinfo    = 0;
$ejectcmd       = "eject";
#
#
########################################################################
#
# No User configurable parameters below here
#
########################################################################
#
require "flush.pl";
use Getopt::Long qw(:config no_ignore_case);
use CDDB_get qw(get_cddb get_discids);
use IO::Socket;
use Fcntl;
#
# Initialize paths.
#
$homedir = "$ENV{HOME}";
$workdir = "$ENV{PWD}";
$usename = "$ENV{USER}";
$hostnam = "$ENV{HOST}";
#print ($_,$ENV{$_},"\n") foreach (keys %ENV);
#
# Initialise global variables
#
$artist	= "";
$artistag	= "";
$artislametag	= "";
$album		= "";
$albumtag	= "";
$albumlametag	= "";
$categ		= "";	# CDDB category
$genreno	= "";	# ID3 genre number
$lameindex	= 0;	# Index of encoder array entry.
$lameflag	= 0;	# Flag to check if lame is used.
$nocddbinfo	= 1;	# Assume no CDDB info exists
$totals	= 0;	# Total length of CD in seconds.
$trackselection = "";# Passed from command line
$sufix		= "";	# Needed in several subroutines
$cddbid	= 0;	# Needed in several subroutines
$cddbgenre	= 0;	# Needed in several subroutines
$tracks	= 0;	# Needed in several subroutines
$tracktag	= "";	# Needed in several subroutines
$tracklametag	= "";	# Needed in several subroutines
@comment	= ();	# Needed in several subroutines
@commentblock	= ();	# Needed in several subroutines
@framelist	= ();	# Needed in several subroutines
@secondlist	= ();	# Needed in several subroutines
@tracklist	= ();	# Needed in several subroutines
@tracktags	= ();	# Needed in several subroutines
$ripcomplete	= 0;	# Needed in several subroutines
$dtitle	= 0;	# Disktitle needed for CDDB submission
%cd		= 0;	# All CDDB entries needed in several subroutines
$cddbsubmission = 2;	# If zero then data for CDDB submission is incorrect
			# if 1: submission OK, if 2: CDDB entry not changed
$revision	= "";	# If >= zero: CDDB entry present, edit file before submission
$wpreset	= "";	# Preset written into config file.
$wcoder	= 0;	# Use a comma seperated string to write the
			# coder-array into the config file.
$wsshlist	= "";	# As above for the list of remote machines.
$sshflag	= 0;	# Ssh encoding OK if sshflag == 1.
%sshlist	= ();	# Hash of remote machines.
$version	= "3.4";
$encstart	= 0;
$ripstart	= 0;
$encend	= 0;
$ripend	= 0;
$riptime	= 0;
$enctime	= 0;
#
# Get the parameters from the command line.
# available:     A       E F G  IjJkK     N O     R           Yz
# already used: a bBcCdDe f g hi     lLmMn o pPqQr sStTuUvVwWy  Z
#
if(! &GetOptions("archive|a!" => \$parchive,
		"bitrate|b=s" => \$pbitrate,
		"maxrate|B=i" => \$pmaxrate,
		"chars|W!" => \$pchars,
		"cddbserver|C=s" => \$PCDDB_HOST,
		"config!" => \$pconfig,
		"coder|c=s" => \@pcoder,
		"comment=s" => \$pcommentag,
		"device|d=s" => \$pcddev,
		"dirtemplate|D=s" => \$pdirtemplate,
		"eject|e!" => \$peject,
		"encode!" => \$pencode,
		"faacopt=s" => \$pfaacopt,
		"flacopt=s" => \$pflacopt,
		"lameopt=s" => \$plameopt,
		"oggopt=s" => \$poggopt,
		"field|f=i" => \$pfield,
		"genre|g=s" => \$pgenre,
		"halt" => \$phalt,
		"help|h" => \$help,
		"interaction|i!" => \$pinteraction,
		"lcd!" => \$plcd,
		"lcdhost=s" => \$plcdhost,
		"lcdport=s" => \$plcdport,
		"lowercase|l!" => \$plowercase,
		"local!" => \$plocal,
		"mirror|m=s" => \$pmirror,
		"mail|M=s" => \$pmailad,
		"multi" => \$pmulti,
		"nice|n=s" => \$pnice,
		"outputdir|o=s" => \$poutputdir,
		"playlist|p!" => \$pplaylist,
		"preset|S=s" => \$ppreset,
		"proxy|P=s" => \$pproxy,
		"protocol|L=i" => \$pproto,
		"quality|q=s" => \@pquality,
		"quiet|Q!" => \$pquiet,
		"rip!" => \$prip,
		"ripper|r=s" => \$pripper,
		"ripopt=s" => \$pripopt,
		"savenew" => \$savenew,
		"save" => \$savepara,
		"scp" => \$pscp,
		"sshlist=s" => \@psshlist,
		"submission|s!" => \$psubmission,
		"tracktemplate|T=s" => \$ptracktemplate,
		"transfer|t=s" => \$ptransfer,
		"underscore|u!" => \$punderscore,
		"utftag|U!" => \$putftag,
		"vbrmode|v=s" => \$pvbrmode,
		"version|V" => \$printver,
		"year|y=i" => \$pyear,
		"wav|w!" => \$pwav,
		"disable-paranoia|Z" => \$pparano ) ) {
	print "\nUsage: ripit [--device|d cd-device] [--outputdir|o path] [--ripper|r cdripper]
             [--ripopt options]  [--wav|w]  [--disable-paranoia|Z]  [--encode]
             [--coder|c encoders-list] [--faacopt options] [--flacopt options]
             [--oggopt options] [--lameopt options] [--quality qualities-list]
             [--bitrate|b rate] [--maxrate|B maxrate] [--vbr-mode|v old / new]
             [--preset|S mode]  [--comment id3-comment]  [--genre|g genre-tag]
             [--lowercase|l] [--underscore|u] [--utftag|U] [--year|y year-tag]
             [--dirtemplate '\"\$parameters\"'] [--tracktemplate '\"\$parameters\"']
             [--cddbserver|C server] [--mirror|m mirror]  [--protocol|L level]
             [--proxy|P path]  [--transfer|t cddb or http]  [--mail|M address]
             [--submission|s] [--interaction|i] [--nice|n adjustment] [--halt]
             [--eject|e] [--help|h]  [--lcd] [--lcdhost host] [--lcdport port]
             [--sshlist remote machines] [--local] [--scp] [--config] [--save]
             [--savenew] [--archive|a] [--playlist|p] [--quiet|Q]  [--chars|W]
             [--version|V] [--rip] [track_selection]
\n
For general usage information type:\n
       ripit --help | less\n
or try to run\n
       ripit\n
without any options.
\n";
	exit 1;
}
#
# Evaluate the command line parameters if passed. We need to do it this
# way, because passed options have to be saved (in case if user wants to
# save them) before config file is read to prevent overriding passed
# options with options from config file. And the passed options shall be
# stronger than the config file options!
# Problems arrise wiht options that can be zero, because zero is equal
# to undef! As one can not distinguish between zero or undef, test it
# if option has a digit in!
#
# First for the normal options.
$bitrate = $pbitrate if($pbitrate);
$maxrate = $pmaxrate if($pmaxrate);
# @coder will be done in the subroutine!
$faacopt = $pfaacopt if($pfaacopt);
$flacopt = $pflacopt if($pflacopt);
$lameopt = $plameopt if($plameopt);
$oggopt = $poggopt if($poggopt);
#
# Because we change $CDDB_HOST save default in $DCDDB_HOST
$CDDB_HOST = $PCDDB_HOST if($PCDDB_HOST);
$DCDDB_HOST = $CDDB_HOST;
$cddev = $pcddev if($pcddev);
$commentag = $pcommentag if($pcommentag);
$dirtemplate = $pdirtemplate if($pdirtemplate);
$field = $pfield if($pfield =~ /^\d/);
$genre = $pgenre if($pgenre);
$halt = $phalt if($phalt);
$lcdhost = $plcdhost if($plcdhost);
$lcdport = $plcdport if($plcdport);
$mirror = $pmirror if($pmirror);
$mailad = $pmailad if($pmailad);
$nice = $pnice if($pnice =~ /^\d/);
$outputdir = $poutputdir if($poutputdir);
$parano = $pparano if($pparano =~ /^\d/);
$preset = $ppreset if($ppreset);
$proto = $pproto if($pproto);
$proxy = $pproxy if($pproxy);
$ripper = $pripper if($pripper =~ /^\d/);
$ripopt = $pripopt if($pripopt);
$scp = $pscp if($pscp =~ /^\d/);
# $psshlist will be done in the subroutine!
$transfer = $ptransfer if($ptransfer);
$tracktemplate = $ptracktemplate if($ptracktemplate);
$vbrmode = $pvbrmode if($pvbrmode);
$year = $pyear if($pyear);
# And for the negatable options.
$archive = $parchive if($parchive =~ /^\d/);
$chars = $pchars if($pchars =~ /^\d/);
$config = $pconfig if($pconfig =~ /^\d/);
$encode = $pencode if($pencode =~ /^\d/);
$eject = $peject if($peject =~ /^\d/);
$interaction = $pinteraction if($pinteraction =~ /^\d/);
$lcd = $plcd if($plcd =~ /^\d/);
$local = $plocal if($plocal =~ /^\d/);
$lowercase = $plowercase if($plowercase =~ /^\d/);
$multi = $pmulti if($pmulti =~ /^\d/);
$playlist = $pplaylist if($pplaylist =~ /^\d/);
$quiet = $pquiet if($pquiet =~ /^\d/);
$rip = $prip if($prip =~ /^\d/);
$submission = $psubmission if($psubmission =~ /^\d/);
$underscore = $punderscore if($punderscore =~ /^\d/);
$utftag = $putftag if($putftag =~ /^\d/);
$wav = $pwav if($pwav =~ /^\d/);
#
# Do some checks before writing the config file.
#
if($help != 1 && $printver != 1) {
	&check_coder();		# Check encoder array.
	&check_quality();		# Check if quality is valid.
	&check_host();		# Check if host is valid.
	&check_proto();		# Check if protocol level is valid.
	&check_sshlist();		# Check list of remote machines.
	&check_preset() if($preset);# Check preset settings.
#
# To be able to overwrite the config file we have to write it before...
#
	&save_config() if($savenew == 1);
#
# ...reading the parameters not yet passed from the config file.
#
	&read_config() if($config == 1);
	&save_config() if($savepara == 1);
	&check_host();	# Do it again to concatenate mirror.host.
	&check_coder();		# Check it again for lame cbr vs vbr.
	&check_sshlist();		# Check it again to create the hash.
}
#
#
########################################################################
#
# MAIN ROUTINE
#
########################################################################
#

#$logger = "/tmp/ripit.log";
#system("echo 'AudioCD rip process started...' > $logger");

textdomain("reel");

open(LOGGER, ">/tmp/ripit.log");
LOGGER->autoflush(1);
print LOGGER gettext("AudioCD rip process started...\n");

#system("touch /tmp/ripit.process >/dev/null 2>&1");

#if ( length($ejectcmd) > 0) {
#	system("$ejectcmd $cddev");
#	sleep 10;
#	system("$ejectcmd -t $cddev");
#	sleep 20;
#}	

print "\nRipIT Version $version beta 20050805\n" if($quiet == 0);
if($printver) {
 	print "\n";
	exit 2;
}

if($help == 1) {
	&print_help();
	exit 3;
}

if($lcd == 1) {			# lcdproc
	&init_lcd();
}

if(($outputdir eq "") && ($output eq "")) {
	$outputdir = $homedir;
	chomp $outputdir;
}

if(($outputdir eq "") && !($output eq "")) {
	$outputdir = $output;
	chomp $outputdir;
}

if($outputdir =~ /^\.\//){		# Change relative paths to full paths
	$outputdir =~ s/^\./$workdir/;
}

#system("echo 'Ausgabeverzeichnis : $outputdir' >>$logger");
#system("echo 'Auslesegeraet : $cddev' >>$logger");
#system("echo '' >>$logger");

if($cddev eq "") {			# Check CD dev if none defined.
	&check_cddev;
}



if(length($year) > 0 && length($year) != 4 ) {
	print STDERR "Warning: year is not Y2K compliant - $year\n" if($quiet == 0);
#	system("echo 'Warnung: Jahr ist nicht Y2K sicher - $year' >>$logger");
	print LOGGER gettext("Warning: year is not Y2K compliant")." - $year\n";
}

if($halt == 1 && $quiet == 0) {
	print "Will halt machine when finished.\n";
#	system("echo 'Computer wird nach Vorgang heruntergefahren' >>$logger");
        print LOGGER gettext("Will halt machine when finished.")."\n"; 
}

if($eject == 1 && $quiet == 0) {
	print "Will eject CD when finished.\n";
	#system("echo 'CD wird nach Rippen ausgeworfen' >>$logger");
	print LOGGER gettext("Will eject CD when finished.")."\n";
}

if($playlist == 0 && $quiet == 0) {
	print "Won't create a m3u file.\n";
#	system("echo 'Playliste wird nicht erstellt' >>$logger");
	print LOGGER gettext("Won't create a m3u file.")."\n";
}

if($utftag == 0 && $quiet == 0) {
	print "Will change encoding of Lame-tags to ISO8859-1.\n";
}

if($wav == 1 && $quiet == 0) {
	print "Will keep the wav files.\n";
#	system("echo 'Wav Dateien werden nicht automatisch geloescht' >>$logger");
        print LOGGER gettext("Will keep the wav files.")."\n";
}

if($ARGV[0] ne '') {			# Get track parameter if given.
	$trackselection = $ARGV[0];
}

if($bitrate != 0 && $lameflag == 1) {	# Check bitrate for Lame.
	&check_bitrate;
}

if($vbrmode ne "" && $lameflag == 1) {	# Check for vbr in Lame.
	&check_vbrmode;
}

if($preset) {
	&lame_preset;
}

&get_cdinfo();			# Extract CD info from CDDB toc.
&create_seltrack($trackselection);	# Create track selection.
&create_dirs();			# Create directories MP3 files.
&rip_cd();				# Rip, Encode & Tag.

if($eject == 1) {
	system "eject $cddev";
}

if($quiet == 0) {
	print "Waiting for Encoder to finish...\n";
#	system("echo 'Warte auf Ende von Encoder...' >>$logger");
        print LOGGER gettext("Waiting for Encoder to finish...")."\n";
}

if($sshflag == 1) {
	&del_wav();
}
else {
	wait;
}

if($playlist == 1) {			# Create m3u file if specified.
	&create_m3u();
}

&del_erlog();
&cal_times();

if(-r "$mp3dir/error.log") {
	print "\nCD may NOT be complete! Check the error.log in $mp3dir!\n\n" if($quiet == 0);
#	system("echo '  ' >>$logger");
#	system("echo 'Vorgang abgebrochen -> error.log in $mp3dir' >>$logger");
#TB: TODO tr following line:
        system("$svdrpcmd MESG 'RIPIT: VORGANG ABGEBROCHEN !'");  
        print LOGGER "  \n";
        print LOGGER gettext("CD may NOT be complete! Check the error.log in")." $mp3dir!\n";
}
else {
	print "\nAll complete!\nRipping needed $riptime min and encoding needed $enctime min.\n\n" if($quiet == 0);
#	system("echo '  ' >>$logger");
        print LOGGER "   \n";
#	system("echo '!! DONE... !! VORGANG BEENDET.' >>$logger");
        print LOGGER gettext("!! DONE... !! ACTION ENDED.")."\n";
#	system("echo '  ' >>$logger");
        print LOGGER "   \n";
        print LOGGER gettext("Ripping:")." $riptime min\n";
        print LOGGER gettext("Encoding:")." $enctime min\n";
#	system("echo 'Rippen  : $riptime min' >>$logger");
#	system("echo 'Encoding: $enctime min' >>$logger");
# TB: TODO translate following line:
        system("$svdrpcmd MESG 'RIPIT: VORGANG FERTIG !'");  

}

if($lcd == 1) {			# lcdproc
	$lcdline1 = " ";
	$lcdline2 = "   RipIT finished   ";
	$lcdline3 = " ";
	&ulcd();
	close($lcdproc) || die "close: $!";
}

system("/bin/rm -f /tmp/ripit.process");

if($halt == 1 && $quiet == 0) {	# Halt when finished.
	print "\nShutdown PC...\n";
#	system("echo '\n\n!! COMPUTER WIRD HERUNTERGEFAHREN !!' >>$logger");
        print LOGGER gettext("Shutdown PC...")."\n";
	system("shutdown -h now");
}

if($multi == 1) {
	open(SRXY,">>$logfile") or print "Can not append to file \"$logfile\"!";
	print SRXY "\nEncoding   ended: $encend";
	close(SRXY);
	open(SUCC,">>$logpath/success.log") or print "Can not append to file \"$logpath/succes.log\"!";
	print "All Complete in $cddev with $artist: $album.\n";
	print SUCC "$artist;$album;$cddbid;$logfile;$hostnam;$riptime;$enctime\n";
	close(SUCC);
}

close(LOGGER);

exit;


########################################################################
#
# SUBROUTINES
#
########################################################################

########################################################################
#
# Read the Album, Artist, DiscID and Track Titles from the get_CDDB()
# generated TOC file.
#
sub get_cdinfo {
	my %config;

	# Get number of ID and number of tracks of CD.
	my $cd = get_discids($cddev);
	my ($id,$toc) = ($cd->[0], $cd->[2]);
	$tracks = $cd->[1];
	$cddbid = sprintf("%08x", $id);

	my $count = 1;
	my $usearch = "x";
	if($archive == 1 && $multi == 0) {
		print "\nChecking for a CDDB entry \@ local machine, please wait a second...\n\n" if($quiet == 0);
#		system("echo 'Suche CDDB Eintrag..., bitte warten...' >>$logger");
                print LOGGER gettext("Searching CDDB entry..., please wait...")."\n";
		system("mkdir -m 0777 -p $homedir/.cddb") && \
#		die "Cannot create directory $homedir/.cddb: $!"||system("echo 'Kann Verzeichnis $homedir/.cddb nicht erstellen...' >>$logger");
                die "Cannot create directory $homedir/.cddb: $!"||print LOGGER gettext("Cannot create")." $homedir/.cddb\n";
		opendir(CDDB, "$homedir/.cddb/") || print "Can not read in $homedir/.cddb: $!";
		my @categs = grep {/\w/i} readdir(CDDB);
		close CDDB;
		my @cddbid = ();
		foreach (@categs) {
			if(-d "$homedir/.cddb/$_") {
				opendir(CATEG, "$homedir/.cddb/$_") || print "Can not read in $homedir/.cddb: $!";
				my @entries = grep {/$cddbid/} readdir(CATEG);
				close CATEG;
				push @cddbid, $_ if($entries[0]);
			}
			elsif(-f "$homedir/.cddb/$_") {
				push @cddbid, $_ if($_ =~ /$cddbid/);
			}
		}
		if($cddbid[0]) {
			print "Found entry $cddbid in $homedir/.cddb on this machine!\n" if($interaction == 1);
			print "This CD could be:\n\n" if($interaction == 1);
#			system("echo 'CDDB Eintrag gefunden : $cddbid' >>$logger");
                        print LOGGER gettext("Found CDDB entry")." : $cddbid\n";
			foreach (@cddbid) {
				my $openflag="no";
				if(-r "$homedir/.cddb/$_/$cddbid") {
					open(LOG, "$homedir/.cddb/$_/$cddbid");
					$openflag = "ok";
				}
				elsif(-r "$homedir/.cddb/$cddbid") {
					open(LOG, "$homedir/.cddb/$cddbid");
					$_ = "none";
					$openflag = "ok";
				}
				if($openflag == "ok") {
					my @loglines = <LOG>;
					close(LOG);
					my @artist = grep(s/^DTITLE=//, @loglines);
					$artist = @artist[0];
					$artist =~ s/[\015]//g;
					chomp $artist;
					my @genre = grep(s/^DGENRE=//, @loglines);
					my $agenre = @genre[0];
					$agenre =~ s/[\015]//g;
					chomp $agenre;
					print "$count: $artist ($agenre) in category: $_\n" if($interaction == 1);
					$count++;
					$agenre = "";
				}
			}
			print "\n0: Use online CDDB instead!\n" if($interaction == 1);
			if($interaction == 0) {
				$usearch = 1;
			}
			else {
				while($usearch !~ /\d/ || $usearch >= $count) {
					print "\nChoose: (1) ";
					$usearch = <STDIN>;
					chomp $usearch;
					$usearch = 1 if($usearch eq "");
				}
			}
		}
		else {
			$usearch = 0;
		}
		if($usearch != 0) {
			&read_entry("$homedir/.cddb/$cddbid[$usearch-1]/$cddbid",$cddbid[$usearch-1],$tracks);
		}
	}
	else {
		$usearch = 0;
	}

	if($usearch == 0) {
		print "\nChecking for a CDDB entry \@ $CDDB_HOST, please wait a second...\n" if($quiet == 0);
#		system("echo 'Suche CDDB Eintrag \@ $CDDB_HOST' >>$logger");
                print LOGGER gettext("Searching CDDB entry")." \@ $CDDB_HOST\n";
		#Configure CDDB_get parameters
		$config{CDDB_HOST} = $CDDB_HOST;
		while($transfer !~ /^cddb$|^http$/) {
			print "Transfer mode not valid!\n";
#			system("echo 'Transfermodus nicht gestattet' >>$logger");
                        print LOGGER gettext("Transfer mode not valid!")."\n";
			print "Enter cddb or http : ";
			$transfer = <STDIN>;
			chomp $transfer;
		}
		if($transfer eq "cddb") {
			$CDDB_PORT = 8880;
			$CDDB_MODE = "cddb";
		}
		elsif($transfer eq "http") {
			$CDDB_PORT = 80;
			$CDDB_MODE = "http";
		}
		$config{CDDB_MODE} = $CDDB_MODE;
		$config{CDDB_PORT} = $CDDB_PORT;
		$config{CD_DEVICE} = $cddev;
		$config{HTTP_PROXY}= $proxy if($proxy);
		if($multi == 1) {
			unless($field) {
				$field = 1;
			}
			if($field == 0) {
				$field = 1;
			}
			$config{entry} = $field;
		}
		if($interaction == 0) {
			$CDDB_INPUT = 0;
		}
		else {
			$CDDB_INPUT = 1;
		}
		$config{input} = $CDDB_INPUT;
		$config{PROTO_VERSION} = $proto;
		#Change to whatever, but be aware to enter exactly 4 words!
		#E.g. username hostname clientname version
		$config{HELLO_ID} = "RipIT www.suwald.com/ripit/ripit.html RipIT $version";

		eval {%cd = get_cddb(\%config);};
		if($@) {
			print "No connection to internet?\n" if($quiet == 0);
#			system("echo 'Keine Verbindung zum Internet' >>$logger");
                        print LOGGER gettext("No connection to internet")."\n";
			$submission = 0;
		}
	}

	# Write CDDB entry to ~/.cddb/category if there is not already
	# an entry present (then $usearch != 0).
	if($archive == 1 && $usearch == 0) {
		$categ = $cd{cat};
		chomp $categ;
		system("mkdir -m 0777 -p $homedir/.cddb/$categ/") && die "Cannot create directory $homedir/.cddb/$categ: $!\n";
		open(TOC, "> $homedir/.cddb/$categ/$cddbid") || print "Can not write to $homedir/.cddb/$categ/$cddbid: $!";
		foreach (@{$cd{raw}}) {
			print TOC $_;
		}
		close TOC;
	}

	if($multi == 1) {
		my @devnameblock = split(/\//, $cddev);
		$logfile = $devnameblock[$#devnameblock];
		&read_entry($logfile);
	}
#		$artist =~ s/[:*#?$\!]//g if($chars == 0);# Strip dodgey chars

	print LOGGER "CDTITLE $cd{title}\n";

	if(defined $cd{title}) {
		$artistag = $cd{artist};
		$albumtag = $cd{title};
		$artistag =~ s/[;|><"\015]//g;		# Strip dodgey chars
		$albumtag =~ s/[;|><"\015]//g;		# Strip dodgey chars
		$artistag =~ s/`/'/g;
		$albumtag =~ s/`/'/g;
		$artist = $artistag;
		$artist = lc($artist) if($lowercase == 1);
		$artist =~ s/ /_/g if($underscore == 1);
		$album = $albumtag;
		$album = lc($album) if($lowercase == 1);
		$album =~ s/ /_/g if($underscore == 1);
		$artist =~ s/[:*#?$\!]//g if($chars == 0);      # Strip dodgey chars
		$album =~ s/[:*#?$\!]//g if($chars == 0);	# Strip dodgey chars
		$artist =~ s/[*]//g;				# Strip dodgey chars
		$album =~ s/[*]//g;				# Strip dodgey chars
		$artist =~ s/\// - /g;
		$album =~ s/\// - /g;
		$dtitle = $artistag . " / " . $albumtag;
		$categ = $cd{cat};
		# Set the year if it wasn't passed on command line.
		if($year eq "") {
			$year = $cd{year};
			$year =~ s/[\015]//g;
		}
		# Set the genre if it wasn't passed on command line.
		# Change the $cddbgenre flag to 1: genre is from CDDB.
		if($genre eq "" && defined $cd{genre} && $multi == 0) {
			$genre = $cd{genre};
			$genre =~ s/[\015]//g;
			$cddbgenre = 1;
		}
		my @revision = grep(/^\#\sRevision:\s/, @{$cd{raw}});
		@revision = grep(s/^\#\sRevision:\s//, @revision);
		$revision = @revision[0];
		chomp $revision;
		# Now extract the disk comment lines EXTD
		# NOTE: Each EXTD line my have \n's, but two EXTD lines do
		# NOT mean that there's a linebreak in between! So, what we
		# have to do is, put all comment lines into a string and
		# split the string according to the \n's. But because I don't
		# like the \n in lines, RipIT will break them and add a \n at
		# the end of each line!
		@comment = grep(/^EXTD=/, @{$cd{raw}});
		@comment = grep(s/^EXTD=//, @comment);
		# clean the comments
		foreach (@comment) {
			chomp $_;
			$_ =~ s/[\015]//g;
		}
		my $line = "@comment";
		$line =~ s/[\015]//g;
		@commentblock = split(/\\n/, $line);
		foreach (@commentblock) {
			chomp $_;
			$_ =~ s/^\s//g;
		}
		# Now extract the track comment lines EXTT
		@trackcom = grep(/^EXTT\d+=/, @{$cd{raw}});
		@trackcom = grep(s/^EXTT\d+=//, @trackcom);
		foreach (@trackcom) {
			chomp $_;
			$_ =~ s/[\015]//g;
		}
		# and extract the playorder line.
		my @playorder = grep(/^PLAYORDER=/, @{$cd{raw}});
		@playorder = grep(s/^PLAYORDER=//, @playorder);
		$playorder = @playorder[0];
		chomp $playorder;
		$nocddbinfo = 0;	# Got CDDB info OK
	}
	else {
		if($submission == 0) {
			print "\nNo CDDB info choosen or found for this CD\n" if($quiet == 0);
#			system("echo 'Keinen CDDB Eintrag gefunden' >>$logger");
                        print LOGGER gettext("No CDDB info choosen or found for this CD")."\n";

		}
		# Set submission OK, will be set to zero if default
		# names are used.
		$cddbsubmission = 1;
		# Don't ask for default settings, use them! ...
		if($interaction == 0) {
			&create_deftrack(1);
		}
		# ... or ask whether 1) default or 2) manual entries
		# shall be used or entered.
		else {
			&create_deftrack(2);
		}
	}
	if($genre ne "" && $multi == 0) {
		&check_genre();
	}
	# What if someone wants underscore and/or lowercase in filenames
	# and has genre in the dir or tracktemplate? So we have to
	# lowercase genre and underscore it if it is a 2 word expression.
	if(($dirtemplate =~ /\$genre/ or $tracktemplate =~ /\$genre/) && $genre ne "") {
		my $tempgenre = $genre;
		$tempgenre = lc($tempgenre) if($lowercase == 1);
		if($genre =~ /\w\s\w/ && $underscore == 1) {
			$tempgenre =~ s/ /_/g;
			# Do it the hard way: assume we have the definitive
			# genre, so replace the string "$genre" with the
			# value of $tempgenre!
			$dirtemplate =~ s/\$genre/$tempgenre/g;
			$tracktemplate =~ s/\$genre/$tempgenre/g;
		}
	}

	if($multi == 1) {
		`sed "2 a Artist - Album:$artist - $album" $logfile > $logfile.new `;
		rename("$logfile.new","$logfile");
		unlink("$logfile.new");
	}

	if($quiet == 0) {
		print "\n-----------------";
#		system("echo '--------------------' >>$logger");
                print LOGGER "--------------------\n";
		print "\nCDDB and ID3 Info";
#		system("echo 'CDDB und ID3 Info' >>$logger");
                print LOGGER gettext("CDDB and ID3 Info")."\n";
		print "\n-----------------\n";
#		system("echo '--------------------' >>$logger");
                print LOGGER "--------------------\n";
		print "Artist: $artistag\n";
                print LOGGER gettext("Artist").": $artistag\n";
#		system("echo 'Artist: $artistag' >>$logger");
		print "Album: $albumtag\n";
                print LOGGER gettext("Album").": $albumtag\n";
#		system("echo 'Album:  $albumtag' >>$logger");
		print "Category: $categ\n";
                print LOGGER gettext("Category").": $categ\n";
#		system("echo 'Kategorie: $categ' >>$logger");
		print "ID3-Genre: $genre ($genreno)\n";
                system LOGGER gettext("Genre").": $genre\n";
#		system("echo 'Genre: $genre' >>$logger");
		if(lc($genre) ne lc($cd{'genre'})) {
			print "CDDB-Genre: $cd{'genre'}\n";
		}
		print "Year: $year\n";
                print LOGGER gettext("Year").": $year\n";
#		system("echo 'Jahr: $year' >>$logger");
		print "Revision: $revision\n";
		# It happens, that the ID from CDDB is NOT
		# identical to the ID calculated from the
		# frames of your CD...
		if($cddbid ne $cd{id} && defined $cd{id} ) {
			print "CDDB id: $cd{id}\n";
		}
		print "CD id: $cddbid\n";
#		system("echo 'CD id: $cddbid' >>$logger");
                print LOGGER gettext("CD id").": $cddbid\n";
		if(defined @commentblock) {
			foreach (@commentblock) {
				print "Comment: $_\n";
#				system("echo 'Kommentar: $_\n' >>$logger");
                                print LOGGER gettext("Comment").": $_\n";
			}
		}
		print "\n";

	}
	# Read out pregap before calculating track lengths.
	my $frames = $toc->[0]->{'frames'};
	push @framelist, "$frames";
	my $n = 1;
	# Now print track infos.
	foreach (@{$cd{track}}) {
		$_ =~ s/`/'/g;	# Change ` to ' to avoid problems later
		$_ =~ s/[;|><"\015]//g;
		push @tracktags, $_;
		# Get frames and total time.
		my $frames = $toc->[$n]->{'frames'};
		push @framelist, "$frames";
		$frames = $frames - $framelist[$n-1];
		my $second = int($frames/75);
		push @secondlist, "$second";
		my $frame = $frames - $second * 75;
		my $minute = int($second/60);
		$second = $second - $minute * 60;
		printf("%02d: [%02d:%02d.%02d] %s\n", $n, $minute, $second, $frame, $_) if($quiet == 0);
		$_ =~ s/[:*#?$\!]//g if($chars == 0);
		$_ =~ s/[*]//g;
		$_ =~ s/\//-/g;	# Change / to - to avoid problems later
		$_ = lc($_) if($lowercase == 1);
		$_ =~ s/ /_/g if($underscore == 1);
		push @tracklist, $_;
		$n++;
	}
	# Get total length from last entry.
	$min = $toc->[$n-1]->{min};
	$sec = $toc->[$n-1]->{sec};
	$totals = $sec + 60 * $min;
	print "\n\n" if($quiet == 0);

	# Some more error checking.
	if(($seltrack[0] - 1) > $#tracklist) {
		die "Starting track is higher than number of tracks on CD!";
	}
	if($artist eq "") {
		die "TOC ERROR: No Artist Found";
	}

	# lcdproc
	if($lcd == 1){
		$lcdline1 = $artist."-".$album;
		$lcdline2 = "R00|00.0%|----------";
		$lcdline3 = "E00|00.0%|----------";
		&ulcd();
	}
}
########################################################################
#
# Create the track selection from the parameters passed on the command-
# line.
#
sub create_seltrack {
	my($i,$tempstr,$intrack);
	($tempstr) = @_;
	if(($tempstr =~ /,/) || ($tempstr =~ /\-/)) {
		@intrack = split(/,/ , $tempstr);
# If last character is a , add an other item with a -
		if($tempstr =~ /,$/) {
			push @intrack,($intrack[$#intack]+1) . "-";
		}
		foreach $intrack (@intrack) {
			if($intrack =~ /\-/) {
				@outrack = split(/-/ , $intrack);
# If last character is a -, add last track to $outrack
				if($#outrack == 0) {
					$outrack[1] = $#tracklist + 1;
				}
				for($i = $outrack[0]; $i <= $outrack[1]; $i++) {
					push @seltrack,$i;
				}
			}
			else {
				push @seltrack,$intrack;
			}
		}
	}
	elsif($tempstr eq '') {
		for($i = 1; $i <= ($#tracklist + 1); $i++) {
			$seltrack[$i - 1] = $i;
		}
	}
	elsif($tempstr =~ /^[0-9]*[0-9]$/) {
		$seltrack[0] = $tempstr;
	}
	else {
		die "Track selection invalid";
	}

	# Sort the tracks in order, perl is so cool :-)
	@seltrack = sort {$a <=> $b} @seltrack;

	# Check the validaty of the track selection.
	foreach (@seltrack) {
		if($_ > ($#tracklist + 1)) {
			die "Track selection higher than no of tracks";
		}
	}
}
########################################################################
#
# Ask if CDDB submission shall be done because one might change some
# settings a last time before writing to directories and files! Then
# create the directory where the flac, mp3 or ogg files will go.
# Fail if directory exists or can't be created.
#
sub create_dirs {
	# Directory created will be: /outputdir/$dirtemplate

	my $index = 2;
	unless($cddbsubmission == 0 || $interaction == 0) {
		while($index !~ /^[0-1]$/) {
			if(defined $cd{title}) {
				print "\nDo you want to edit the CDDB entry? Try it, it's easy!\n\n";
			}
			else {
				print "\nDo you want to submit your entries to the CDDB?";
				print "\nYou're almost there, just type Enter at each question!\n\n";
			}
			print "1: Yes, and I know about the naming-rules of freedb.org \n\n";
			print "0: No\n\nChoose [0-1]: (0) ";
			$index = <STDIN>;
			chomp $index;
			if($index eq "") {
				$index = 0;
			}
			print "\n";
		}
		if($index == 1) {
			if($revision ne "") {
				print "\nPlease change some settings!";
				print "\nYou may confirm all CDDB settings with Enter!\n";
				$revision++;
				&create_deftrack(0);
			}
			else{
				$revision = 0;
			}
		}
		elsif($index != 0) {
			print "You should choose 0 or 1!\n";
		}
	}
	if($index == 1) {
		&pre_subm();
	}

	# Now check and create the full path "mp3dir" where the files
	# will go. First Check the dirtemplate and use the actual year
	# as default if none is given!
	if(($dirtemplate =~ /\$year/ || $tracktemplate =~ /\$year/) && $year eq "") {
		$year = `date \'+%Y\'`;
		chomp $year;
	}
	if(($dirtemplate =~ /\$genre/ || $tracktemplate =~ /\$genre/) && $genre eq "") {
		$genre = Unknown;
		chomp $genre;
	}
	if(!eval("\$dir = $dirtemplate")) {
		die "Directory Template incorrect, caused eval to fail: $!";
	}

	$dir = lc $dir if($lowercase == 1);
	$dir =~ s/ /_/g if($underscore == 1);

	$mp3dir = $outputdir."/".$dir;
	# Delete ending . in directory name if no special characters
	# wanted!
	$mp3dir =~ s/[.]+$// if($chars == 0);
	# Now check if the outputdir alread exists, if it does, try
	# "outputdir i" with i an integer until it works.
	my $i=1;
	my $nmp3dir = $mp3dir;
	if($multi == 0) {
		while(defined(opendir(TESTDIR, $nmp3dir)) && $rip == 1) {
			$nmp3dir = $mp3dir." ".$i if($underscore == 0);
			$nmp3dir = $mp3dir."_".$i if($underscore == 1);
			$i++;
		}
	}
	else {
		if(defined(opendir(TESTDIR, $mp3dir))) {
			# Delete error.log now, else the next device will
			# start to encode!
			if(-r "$mp3dir/error.log") {
				print "Found an orfand error.log in $mp3dir.\n";
				unlink("$mp3dir/error.log");
			}
			`echo " " >> $logfile`;
			`echo "The directory $mp3dir is already present!" >> $logfile`;
			# We supposed it would be better to write into the same
			# directory, if identical CDDB entry for another CD found.
			# But there are plenty of Compilations with more than one
			# disc which have all the same title...
			while(defined(opendir(TESTDIR, $nmp3dir))) {
				$nmp3dir = $mp3dir." ".$i if($underscore == 0);
				$nmp3dir = $mp3dir."_".$i if($underscore == 1);
				$i++;
			}
			if(-r "$nmp3dir/error.log") {
				print "Found an orfand error.log in $nmp3dir.\n";
				unlink("$nmp3dir/error.log");
			}
			$i--;
			`sed '3 s/\$/ $i/' $logfile > $logfile.new ` if($underscore == 0);
			`sed '3 s/\$/_$i/' $logfile > $logfile.new ` if($underscore == 1);
			rename("$logfile.new","$logfile");
			unlink("$logfile.new");
		}
		my @logpath = split(/\//, $mp3dir);
		pop(@logpath);
		pop(@logpath);
		$logpath = join('/',@logpath);
	}
	$mp3dir = $nmp3dir."/";

	if(!opendir(TESTDIR, $mp3dir)) {
		system("mkdir -m 0777 -p \"$mp3dir\"") && \
			die "Cannot create directory $mp3dir: $!";
	}
	else {
		closedir(TESTDIR);
	}
}
########################################################################
#
# Create the track file name from the tracktemplate variable.
#
sub get_trackname {
	my($trnum,$trname,$riptrname);

	($trnum,$trname) = @_;

	# Create the full file name from the track template, unless
	# the disk is unknown.
	if($nocddbinfo == 0) {
		# We do not need to lowercase the tracktemplate, because
		# all variables in are already lowercased!
		$tracktemplate =~ s/ /\\_/g if($underscore == 1);
		# We have to update tracknum and trackname because they
		# are evalueted by the tracktemplate!
		$tracknum = sprintf("%02d",$trnum);
		$trackname = $trname;
		$trackname =~ s/[:*#?$\!\"\']//g if($chars == 0);      # Strip dodgey chars
		if(!eval("\$riptrname = \$mp3dir.$tracktemplate")) {
			die "Track Template incorrect, caused eval to fail: $!";
		}
	}
	else {
		$trname  = lc $trname if($lowercase == 1);
		$trname =~ s/ /_/g if($underscore == 1);
		$riptrname = $mp3dir.$trname;
	}

	return $riptrname;
}
########################################################################
#
# Rip the CD.
#
sub rip_cd {
	my($shortname,@shortname,$startenc,$trackcn);
	
	if($#seltrack == 0) {
		print "Track @seltrack will be ripped!\n\n" if($quiet == 0);
#		system("echo 'Track @seltrack wird ausgelesen\n\n' >>$logger");
                print LOGGER gettext("Track")." @seltrack ".gettext("will be ripped")."\n";
	}
	else {
		print "Tracks @seltrack will be ripped!\n\n" if($quiet == 0);
#		system("echo 'Tracks @seltrack werden ausgelesen\n\n' >>$logger");
                print LOGGER gettext("Tracks")." @seltrack ".gettext("will be ripped")."\n";
	}
	$trackcn = 0;

	$ripstart = `date \'+%R\'`;
	chomp $ripstart;

	if($multi == 1) {
		open(SRXY,">>$logfile") or print "Can not append to file \"$logfile\"!";
		print SRXY "\nRipping  started: $ripstart";
		close(SRXY);
	}

#	system("echo 'Rippen gestartet' >>$logger");
        print LOGGER gettext("Ripping started")."\n";

	foreach (@seltrack) {
		$trackcn++;
		$riptrackname = &get_trackname($_, $tracklist[$_ - 1]);
		$riptrackno = $_;
		@shortname = split(/\// , $riptrackname);
		$shortname = $shortname[$#shortname];
		print "Ripping \"$shortname\"...\n" if($quiet == 0);
		$shortname =~ s/[:*#?$\!\"\']//g if($chars == 0);      # Strip dodgey chars
#		system("echo 'Ripper  : \"$shortname\".....' >>$logger");
                print LOGGER gettext("Ripper")." : $shortname\n";
		# lcdproc
		if($lcd == 1){
			$_lcdtracks = scalar @seltrack;
			$lcdtrackno++;
			if($_lcdtracks eq $lcdtrackno) {
				$lcdperc = "*100";
			}
			else {
				$lcdperc = sprintf("%04.1f",$lcdtrackno/$_lcdtracks*100);
			}
			$lcdline2=~ s/\|\d\d.\d/\|$lcdperc/;
			$lcdtracknoF = sprintf("%02d",$lcdtrackno);
			$lcdline2=~ s/\R\d\d/\R$lcdtracknoF/;
			substr($lcdline2,10,10) = substr($shortname,3,13);
			&ulcd();
		}
		# There is a problem with too long file names,
		# encountered e.g. with some classical CDs.
		# Cdparanoia cuts the length, cdda2wav too...
		# but how should RipIT know?
		# Use a shorter track name if total length (incl.
		# path) > 230 characters.
		if(length($riptrackname) > 230) {
			$riptrackname = &get_trackname($_,$_."short");
		}
		# Choose the cdaudio ripper to use
		#
		# TODO: Check behaviour of all rippers when data track
		# encountered. Choose to use print instead of die if
		# ripper stops itself!
		# dagrab fails @ data-track, so don't die! and create an error.log
		# cdparanoia fails @ data-track, so don't die! and create an error.log
		# cdda2wav prints errors @ data-track, therefore die!
		if($ripper == 0 && $rip == 1) {
			if($trackcn == 1) {
				$ripopt = $ripopt . " -r 3" if($parano == 1);
				$ripopt = $ripopt . " -v" if($quiet == 0);
			}
			if(system("(dagrab $ripopt -d $cddev -f \"$riptrackname.rip\" $riptrackno 3>&1 1>&2 2>&3 | tee -a \"$mp3dir/error.log\" ) 3>&1 1>&2 2>&3 ")) {
				print "Dagrab detected some read errors on $tracklist[$_ - 1]\n\n";
				# Create error message in CD-directory for encoder: don't wait.
				open(ERO,">>$mp3dir/error.log") or print "Can not append to file \"$mp3dir/error.log\"!";
				print ERO "\nDagrab detected some read errors at $riptrackno on CD $artist - $album, do not worry!";
				close(ERO);
			}
			print "\n";
		}
		elsif($ripper == 1 && $rip == 1) {
			if($trackcn == 1) {
				$ripopt = $ripopt . " -Z" if($parano == 1);
				$ripopt = $ripopt . " -q" if($quiet == 1);
			}
			if($multi == 0) {
				if(system("cdparanoia $ripopt -d $cddev $riptrackno \"$riptrackname.rip\"")) {
					print "cdparanoia failed on $tracklist[$_ - 1]\n\n";
					# Create error message in CD-directory for encoder: don't wait.
#					system("echo 'cdparanoia fehlgeschlagen! Track [$_ - 1]' >>$logger");
                                        print LOGGER gettext("cdparanoia failed on")." $tracklist[$_ - 1]\n";
					open(ERO,">>$mp3dir/error.log") or print "Can not append to file \"$mp3dir/error.log\"!";
					print ERO "\nTrack $riptrackno on CD $artist - $album failed!";
					close(ERO);
				}
			}
			elsif($multi == 1) {
				$ripopt = $ripopt . " -X" if($trackcn == 1);
				if(system("cdparanoia $ripopt -d $cddev $riptrackno \"$riptrackname.rip\" 2>> \"$logfile.$riptrackno.txt\"")) {
					# Apend error message to file srXY for rip2m to start checktrack.
					open(SRXY,">>$logfile") or print "Can not append to file \"$logfile\"!";
					print SRXY "\ncdparanoia failed on $tracklist[$_ - 1] in device $cddev";
					close(SRXY);
					# Create error message in CD-directory for encoder: don't wait.
					open(ERO,">>$mp3dir/error.log") or print "Can not append to file \"$mp3dir/error.log\"!";
					print ERO "Track $riptrackno on CD $artist - $album failed!\n";
					close(ERO);
					# Kill failed CD only if it is not the last track. Last track may be data/video track.
					# I.e. print error message to file srXY.Z.txt, checktrack will grep for string
					# "cdparanoia failed" and kill the CD immediately!
					if($riptrackno != $seltrack[$#seltrack]) {
						open(SRTF,">>$logfile.$riptrackno.txt") or print "Can not append to file \"$logfile.$riptrackno.txt\"!";
						print SRTF "\ncdparanoia failed on $tracklist[$_ - 1] in device $cddev, error !";
						close(SRTF);
						# Create on the fly error message in log-directory.
						open(ERO,">>$logpath/failed.log") or print "Can not append to file \"$logpath/failed.log\"!";
						print ERO "$artist;$album;$cddbid;$cddev;$hostnam\n";
						close(ERO);
						# Now wait to be terminated by checktrack.
						sleep 360;
						exit;
					}
				}
			}
		}
		elsif($ripper == 2 && $rip == 1) {
			if($trackcn == 1) {
				$ripopt = $ripopt . " -q" if($quiet == 1);
			}
			if($multi == 0) {
				if(system("cdda2wav -D $cddev -H $ripopt -t $riptrackno \"$riptrackname\_rip\"")) {
					print "cdda2wav failed on $tracklist[$_ - 1]";
					open(ERO,">>$mp3dir/error.log") or print "Can not append to file \"$mp3dir/error.log\"!";
					print ERO "\nTrack $riptrackno on CD $artist - $album failed!";
					close(ERO);
				}
			}
			elsif($multi == 1) {
				if(system("cdda2wav -D $cddev -H $ripopt -t $riptrackno \"$riptrackname\_rip\" 2>> \"$logfile.$riptrackno.txt\"")) {
					# Apend error message to file srXY for rip2m to start checktrack.
					open(SRXY,">>$logfile") or print "Can not append to file \"$logfile\"!";
					print SRXY "\ncdda2wav failed on $tracklist[$_ - 1] in device $cddev";
					close(SRXY);
					# Create error message in CD-directory for encoder: don't wait.
					open(ERO,">>$mp3dir/error.log") or print "Can not append to file \"$mp3dir/error.log\"!";
					print ERO "Track $riptrackno on CD $artist - $album failed!\n";
					close(ERO);
					# Kill failed CD only if it is not the last track. Last track may be data/video track.
					# I.e. print error message to file srXY.Z.txt, checktrack will grep for string
					# "cdparanoia failed" and kill the CD immediately!
					if($riptrackno != $seltrack[$#seltrack]) {
						open(SRTF,">>$logfile.$riptrackno.txt") or print "Can not append to file \"$logfile.$riptrackno.txt\"!";
						print SRTF "\ncdparanoia failed on $tracklist[$_ - 1] in device $cddev, error !";
						close(SRTF);
						# Create on the fly error message in log-directory.
						open(ERO,">>$logpath/failed.log") or print "Can not append to file \"$logpath/failed.log\"!";
						print ERO "$artist;$album;$cddbid;$cddev;$hostnam\n";
						close(ERO);
						# Now wait to be terminated by checktrack.
						sleep 360;
						exit;
					}
				}
			}
		}
		elsif($ripper == 3 && $rip == 1) {
			if(system("tosha -d $cddev -f wav -t $riptrackno -o \"$riptrackname.rip\"")) {
				die "tosha failed on $tracklist[$_ - 1]";
			}
		}
		elsif($ripper == 4 && $rip == 1) {
			$cdd_dev = $cddev;
			$cdd_dev =~ s/^\/dev\/r//;
			$cdd_dev =~ s/c$//;
			if(system("cdd -t $riptrackno -q -f $cdd_dev - 2> /dev/null | sox -t cdr -x - \"$riptrackname.rip\"")) {
				die "cdd failed on $tracklist[$_ - 1]";
			}
		}
		elsif($rip == 1) {
			print "No CD Ripper defined";
		}

		# Rename rip file to a wav for encoder so that it will be picked
		# up by the encoder background process.
		# Cdda2wav output is not easy to handle.
		# Everything beyond the last . has been erased.
		if($ripper == 2) {
			if($riptrackname =~ /\./) {
				# And split is too clever! If a trackname
				# ends with "name..." all points get lost,
				# so we've to add a word at the end!
				my $cddatrackname = $riptrackname . "end";
				@riptrackname = split(/\./, $cddatrackname);
				delete($riptrackname[$#riptrackname]);
				$cddatrackname = join('.',@riptrackname);
				rename("$cddatrackname.wav","$riptrackname.rip");
			}
			else {
				rename("$riptrackname\_rip.wav","$riptrackname.rip");
			}
		}
		rename("$riptrackname.rip","$riptrackname.wav");
		unlink("$logfile.$riptrackno.txt") if($multi == 1);
#		print "Rip complete $tracklist[$_ - 1]\n\n";

		# Start the encoder in the background, but only once.
		if($startenc == 0 && $encode == 1) {
			$encstart = `date \'+%R\'`;
			chomp $encstart;
			if($multi == 1) {
				open(SRXY,">>$logfile") or print "Can not append to file \"$logfile\"!";
				print SRXY "\nEncoding started: $encstart";
#				system("echo 'Encoden gestartet: $encstart' >>$logger");
                                print LOGGER gettext("Encoding started").": $encstart\n";
				close(SRXY);
			}
			$startenc = 1;
			unless(fork) {
				print "Start encoding now!\n";
#				system("echo 'Encoden gestartet' >>$logger");
                                print LOGGER gettext("Encoding started")."\n";
				&enc_cd();
			}
		}
		# Make output look nice. Keep ENCODER message together!
		if($riptrackno != $seltrack[$#seltrack]) {
			sleep 1;
		}
	}
	# Ugly hack to tell the child process that we are waiting for it
	# to finish.
	open(ERR, ">>$mp3dir/error.log") || print "Can not append to file error.log!\n";
	print ERR "The audio CD ripper reports: all done!\n";
	close(ERR);
	$ripend = `date \'+%R\'`;
	chomp $ripend;
	if($multi == 1) {
		open(SRXY,">>$logfile") or print "Can not append to file \"$logfile\"!";
		print SRXY "\nRipping complete: $ripend";
#		system("echo 'Rippen komplett !! $ripend' >>$logger");
                print LOGGER gettext("Ripping complete").": $ripend\n";
		close(SRXY);
	}
}
########################################################################
#
# Encode the wav
# This runs as a separate process from the main program which
# allows it to continuously encode as the ripping is being done.
# The encoder will also wait for the ripped wav in-case the encoder
# is faster than the CDROM.
#
sub enc_cd {
	my($x,$enc,$ncount,$riptrackno,$riptrackname);

	foreach (@seltrack) {
		$riptrackname = &get_trackname($_, $tracklist[$_ - 1]);
		$riptrackno = $_;
		$ncount++;
		# lcdproc
		if($lcd == 1){
			$_lcdtracks = scalar @seltrack;
			$_lcdenctrack = $ncount;
			if($_lcdtracks eq $_lcdenctrack) {
				$lcdperc = "*100";
			}
			else {
				$lcdperc = sprintf("%04.1f",$_lcdenctrack/$_lcdtracks*100);
			}
			$lcdline3=~ s/\|\d\d.\d/\|$lcdperc/;
			$_lcdenctrackF = sprintf("%02d",$_lcdenctrack);
			$lcdline3=~ s/\E\d\d/\E$_lcdenctrackF/;
			my @shortname = split(/\// , $riptrackname);
			my $shortname = $shortname[$#shortname];
			substr($lcdline3,10,10) = substr($shortname,3,13);
			&ulcd();
		}

		# Cosmetics for nice output.
		my $lasttrack = $seltrack[$#seltrack];
		my $lasttrackname = &get_trackname($lasttrack, $tracklist[$lasttrack - 1]);
		my @shortname = split(/\// , $riptrackname);
		my $shortname = $shortname[$#shortname];

		$tracktag = $tracktags[$_ - 1];
		if($utftag == 0) {
#			$tracktag = utf8::downgrade($tracktag)
#			$artistag = utf8::downgrade($artistag)
#			$albumtag = utf8::downgrade($albumtag)
			$tracklametag = &back_encoding($tracktag);
			$artislametag = &back_encoding($artistag);
			$albumlametag = &back_encoding($albumtag);
		}
		else{
			$tracklametag = $tracktag;
			$artislametag = $artistag;
			$albumlametag = $albumtag;
		}

		# Keep looping until the wav file appears, i.e. wait for
		# ripper timeout. Timeout is 3 times the length of track
		# to rip/encode. Then leave that one and finish the job!
		my $slength = $secondlist[$_ - 1];
		my $mlength = (int($slength / 60) + 1) * 3;
		my $tlength = (int($slength / 10) + 6) * 3;
		my $x = 0;
		# If the file name is too long, look for special
		# name.
		my $wavname = $riptrackname;
		if(length($riptrackname) > 230) {
			$wavname = &get_trackname($_,$_."short");
		}
		while(! -r "$wavname.wav") {
			$x++;
			last if($x > $tlength);
			# Condition 1: Too long waiting for the track!
			if($x >= $tlength) {
				print "Encoder waited $mlength minutes for file\n";
				print "$shortname.wav to appear, now giving up!\n";
				print "with $artist - $album in device $cddev\n" if($multi == 1);
#				system("echo 'Encode wartet seit $mlength Mintuen auf Datei' >>$logger");
                                print LOGGER gettext("Encoder waited")." $mlength ".gettext("minutes for file")."\n"; 
				$shortname =~ s/[:*#?$\!\"\']//g if($chars == 0);      # Strip dodgey chars
#				system("echo 'Encoder: Gebe auf -> \"$shortname.wav\"' >>$logger");
                                print LOGGER gettext("Encoder: giving up")." -> \"$shortname.wav\"\n";
				$x = $tlength + 1;
				if($multi == 1){
					$x = 0 if(-r "$riptrackname.rip");
					print "Don't worry, I continue the job!\n\n";
					
				}
			}
			sleep 10;
			# Condition 2: Check the error log!
			# If at this moment the ripper did not start with
			# the riptrackname.rip, assume it was a data track!
			# If cdparanoia failed on a data track, there will
			# be an entry in the error.log.
			# If dagrab gave error messages, but the wav file
			# was created, we won't get to this point, so don't
			# worry.
			if(-r "$mp3dir/error.log") {
				open(ERR, "$mp3dir/error.log") || print "error.log disappeared!\n";
				my @errlines = <ERR>;
				close ERR;
				my @errtrack = grep(/^Track $riptrackno /, @errlines);
				my $errtrack = "@errtrack";
				@errtrack = split(/ /, $errtrack);
				$errtrack = @errtrack[1];
				if($errtrack) {
					$x = $tlength + 1;
					print "\nDid not detect track $errtrack ($shortname.rip), assume ripper failure!\n" if($quiet == 0);
					print "I will finish the job! Check the error.log!\n" if($quiet == 0 && $sshflag == 0);
				}
			}
		}
		next if($x > $tlength);
		if(length($riptrackname) > 230) {
			rename("$wavname.wav","$riptrackname.wav");
		}

		my $delwav = 0;
		# Set the encoder(s) we are going to use.
		for($c=0; $c<=$#coder; $c++) {
			# This file exists if the ripper finished. It is used to enhance
			# the output. Don't mess up the messages!
			if($ripcomplete == 0 ) {
				if(-r "$mp3dir/error.log") {
					open(ERR, "$mp3dir/error.log") || print "Can not open file error.log!\n";
					my @errlines = <ERR>;
					close ERR;
					my @ripcomplete = grep(/^The audio CD ripper reports: all done!$/, @errlines);
					$ripcomplete = 1 if(@ripcomplete);
				}
			}
			if($ripcomplete == 1 && $quiet == 0) {
				print "\n";
			}
			if($ncount == 1 && $ripcomplete == 0 && $c > 0 && $quiet == 0) {
				print "\n\n";
			}
			if($ncount > 1 && $ripcomplete == 0 && $quiet == 0) {
				print "\n\n";
			}
			my $sufix = "";
			# Get the command for the encoder to use!
			if($coder[$c] == 0) {
				if($ncount == 1) {
					if($preset) {
						$lameopt = $lameopt . " --preset $preset";
					}
					else
					{
						$lameopt = $lameopt . " --vbr-$vbrmode" if($vbrmode);
						$lameopt = $lameopt . " -b $bitrate" if($bitrate ne "off");
						$lameopt = $lameopt . " -B $maxrate" if($maxrate != 0);
						$lameopt = $lameopt . " -V $qualame" if($qualame ne "off" && $vbrmode);
						$lameopt = $lameopt . " -q $qualame" if($qualame ne "off" && !$vbrmode);
					}
				}
				$enc = "lame $lameopt -S --tt \"$tracklametag\" --ta \"$artislametag\" --tl \"$albumlametag\" --ty \"$year\" --tg \"$genre\" --tn $riptrackno --tc \"$commentag\" --add-id3v2 \"$riptrackname.wav\" \"$riptrackname.mp3\"";
				print "Lame $lameopt encoding track ".$ncount." of ".($#seltrack + 1)."" if($quiet == 0);
				print "\@ quality $qualame\n" if($quiet == 0 && !$preset);
				print "\n" if($quiet == 0);
				$sufix = "mp3";
			}
			elsif($coder[$c] == 1) {
				if($ncount == 1) {
					$oggopt = $oggopt . " -q $qualogg" if($qualogg ne "off");
					$oggopt = $oggopt . " -M $maxrate" if($maxrate != 0);
				}
				$enc = "oggenc $oggopt -t \"$tracktag\" -a \"$artistag\" -l \"$albumtag\" -d \"$year\" -G \"$genre\" -N $riptrackno -c \"$commentag\" -o \"$riptrackname.ogg\" \"$riptrackname.wav\"";
				print "Ogg $oggopt encoding track ".$ncount." of ".($#seltrack + 1)." \@ quality $qualogg\n" if($quiet == 0);
				$sufix = "ogg";
			}
			elsif($coder[$c] == 2) {
				if($ncount == 1) {
					$flacopt = $flacopt . " -$quaflac" if($quaflac ne "off");
				}
				$enc = "flac $flacopt --tag=Title=\"$tracktag\" --tag=Artist=\"$artistag\" --tag=Album=\"$albumtag\" --tag=Year=\"$year\" --tag=Catagory=\"$categ\" --tag=Genre=\"$genre\" --tag=Tracknumber=\"$riptrackno\" --tag=Comment=\"$commentag\" --tag=CD_id=\"$cddbid\" \"$riptrackname.wav\"";
				print "Flac $flacopt encoding track ".$ncount." of ".($#seltrack + 1)." \@ compression $quaflac\n" if($quiet == 0);
				$sufix = "flac";
			}
			elsif($coder[$c] == 3) {
				if($ncount == 1) {
					$faacopt = $faacopt . " -q $quafaac" if($quafaac ne "off");
				}
				$enc = "faac $faacopt -w --title \"$tracktag\" --artist \"$artist\" --album \"$album\" --year \"$year\" --genre \"$genre\" --track $riptrackno --comment \"$commentag\" \"$riptrackname.wav\" ";
				print "Faac $faacopt encoding track ".$ncount." of ".($#seltrack + 1)." \@ quality $quafaac\n" if($quiet == 0);
				$sufix = "m4a";
			}
			$sufix = "mp3" if($sufix eq "");
			# Set "last encoding of track" - flag.
			$delwav = 1 if($wav == 0 && $c == $#coder);
			# Set nice if wished.
			$enc="nice -n $nice ".$enc if($nice);
			print "Encoding \"$shortname\"...\n" if($quiet == 0);
			$shortname =~ s/[:*#?$\!\"\']//g if($chars == 0);      # Strip dodgey chars
#			system("echo 'Encoder : \"$shortname\"...' >>$logger");
                        print LOGGER gettext("Encoder")." : $shortname...\n";
			# Make the output look nice, don't mess the messages!
			if($ripcomplete == 0 ) {
				if(-r "$mp3dir/error.log") {
					open(ERR, "$mp3dir/error.log") || print "Can not open file error.log!\n";
					my @errlines = <ERR>;
					close ERR;
					my @ripcomplete = grep(/^The audio CD ripper reports: all done!$/, @errlines);
					$ripcomplete = 1 if(@ripcomplete);
				}
				print "\n" if($quiet == 0);
			}
			# Finally, do the job of encoding, and a lot of cosmetics...
			if($sshflag == 1) {
				&enc_ssh($delwav,$enc,$riptrackname,$shortname,$sufix);
			}
			else {
				if(! system("$enc > /dev/null 2> /dev/null")) {
					if($ripcomplete == 0) {
						if(-r "$mp3dir/error.log") {
							open(ERR, "$mp3dir/error.log") || print "Can open file error.log!\n";
							my @errlines = <ERR>;
							close ERR;
							my @ripcomplete = grep(/^The audio CD ripper reports: all done!$/, @errlines);
							$ripcomplete = 1 if(@ripcomplete);
						}
						print "\n\n" if($quiet == 0);
					}
					print "Encoding complete \"$shortname\"\n" if($quiet == 0);
					$shortname =~ s/[:*#?$\!\"\']//g if($chars == 0);      # Strip dodgey chars
#					system("echo 'Encoder: Fertig -> \"$shortname\"' >>$logger");
                                        print LOGGER gettext("Encoder: finished")." -> \"$shortname\"\n";
					if($ripcomplete == 0) {
						if(-r "$mp3dir/error.log") {
							open(ERR, "$mp3dir/error.log") || print "Can not open file error.log!\n";
							my @errlines = <ERR>;
							close ERR;
							my @ripcomplete = grep(/^The audio CD ripper reports: all done!$/, @errlines);
							$ripcomplete = 1 if(@ripcomplete);
						}
						print "\n" if($quiet == 0);
					}
				}
				else {
					print "Encoder failed on $tracklist[$_ - 1] in $cddev.";
#					system("echo 'Encoder : FEHLGESCHLAGEN -> $tracklist[$_ - 1] -> $cddev' >>$logger");
                                        print LOGGER gettext("Encoder failed on")." $tracklist[$_ - 1]\n";
					if($multi == 1) {
						# Print error message to file srXY.Z.txt, checktrack will grep
						# for string "encoder failed" and kill the CD immediately!
						open(SRTF,">>$logfile.$riptrackno.txt") or print "Can not append to file \"$logfile.$riptrackno.txt\"!";
						print SRTF "\nencoder failed on $tracklist[$_ - 1] in device $cddev, error !";
						close(SRTF);
						# Create on the fly error message in log-directory.
						open(ERO,">>$logpath/failed.log") or print "Can not append to file \"$logpath/failed.log\"!";
						print ERO "$artist;$album;$cddbid;$cddev;$hostnam!\n";
						close(ERO);
						# Now wait to be terminated by checktrack.
						sleep 360;
					}
				}
				sleep 1;
			}
		}
		if($delwav == 1 && $sshflag == 0) {
			unlink("$riptrackname.wav");
		}
	}
	exit ;
}
########################################################################
#
# Creates the M3U file used by players such as X11Amp.
#
sub create_m3u {
	my ($file);
	my @mp3s = ();

	$file = "$artist.m3u";
	if($underscore == 1) {
		$file =~ s/ /_/g;
	}

	opendir(DIR, $mp3dir);
	open(PLIST, ">$mp3dir$file");
	@mp3s = readdir(DIR);

	# TODO: it works for the moment, but one should consider that
	# coder is an array, and for which files should a m3u file be
	# done? only mp3, only ogg ?
	if($coder == 1) {
		@mp3s = grep { /\.ogg/i } @mp3s;
	}
	elsif($coder == 2) {
		@mp3s = grep { /\.flac/i } @mp3s;
	}
	else {
		@mp3s = grep { /\.mp3/i } @mp3s;
	}

	# Now start writing to the file:
	print PLIST "#EXTM3U\n";
	#
	# Remember to write according to the trackselection, i.e.
	# don't write the names from first track on to the m3u file!
	my $i = 0;
	foreach (@mp3s) {
		my $n = $seltrack[$i] - 1;
		# Use tags to write into m3u file, not filenames!
		if($tracktags[$n]) {
			print PLIST "#EXTINF:$secondlist[$n],$tracktags[$n]\n";
		}
		# Use filenames for m3u file only if no tags found!
		# This should not happen!
		else {
			$riptrackname = &get_trackname($seltrack[$i], $tracklist[$_ - 1]);
			my @shortname = split(/\// , $riptrackname);
			my $shortname = $shortname[$#shortname];
			$shortname =~ s/^\d\d[\s_]//;
			if($underscore == 1) { $shortname =~ s/_/ /g; };
			print PLIST "#EXTINF:$secondlist[$n],$shortname\n";
		}
		print PLIST $mp3dir, $_, "\n";
		$i++;
	}

	#print PLIST "$_\n" for reverse (@mp3s);
	close(PLIST);
	closedir(DIR);
}
########################################################################
#
# Creates a default or manual track list.
#
sub create_deftrack {
# Chose if you want to use default names or enter them manually.
# Do not ask if we come form CDDB submission, i.e. index == 0,
# or if $interaction == 0, then $index == 1.
	my($i,$j,$index,$test) = (0,1,@_);

	$index = 1 if($quiet == 1);

	while($index !~ /^[0-1]$/ ) {
		print "\nThis CD shall be labeled with:\n\n";
		print "1: Default Album, Artist and Tracknames\n\n";
		print "0: Manual input\n\nChoose [0-1]: (0) ";
		$index = <STDIN>;
		chomp $index;
		if($index eq "") {
			$index = 0;
		}
		print "\n";
	}
# Create default tracklist and cd-hash.
	if($index == 1) {
		$artist = "Unknown Artist";
		$album = "Unknown Album";
		%cd = (
			artist => $artist,
			title => $album,
			cat => $cat,
			genre => $genre,
			year => $year,
		);
		while($i < $tracks) {	# Treat $tracks as scalar
			$j = $i + 1;
			$j = "0" . $j if($j < 10);
			$cd{track}[$i] = "Track "."$j";
			++$i;
		}
		$cddbsubmission = 0;
	}
# Create manual tracklist.
	elsif($index == 0) {
		$test="a"x257;
		while(length($test) > 256) {
			# In case of CDDB resubmission
			if(defined $cd{artist}) {
				print "\n   Artist ($artist): ";
			}
			# In case of manual CDDB entry
			else {
				print "\n   Artist : ";
			}
			$artist = <STDIN>;
			chomp $artist;
			$test = $artist;
			if(length($test) > 256) {
				print "\nComment should not be longer than 256 characters.";
				print "\nPlease enter the artist name again! \n";
			}
		}
		# If CDDB entry confirmed, take it
		if(defined $cd{artist} && $artist eq "") {
			$artist = $cd{artist};
		}
		# If CDDB entry CHANGED, submission OK
		elsif(defined $cd{artist} && $artist ne "") {
			$cddbsubmission = 1;
		}
		if($artist eq "") {
			$artist = "Unknown Artist";
			$cddbsubmission = 0;
		}
		$artist =~ s/`/'/g;	# Change ` to ' to avoid problems later
		$artist =~ s/[;|><"\015]//g;
		$artistag = $artist;
		$artist =~ s/[:*#?$\!]//g if($chars == 0);
		$artist =~ s/[*]//g;
		$artist =~ s/\//-/g;	# Change / to - to avoid problems later
		$test="a"x257;
		while(length($test) > 256) {
			if(defined $cd{title}) {
				print "\n   Album ($album): ";
			}
			else {
				print "\n   Album : ";
			}
			$album = <STDIN>;
			chomp $album;
			$album =~ s/[;:*#|><"\015]//g;	# Don't count these caracters
			$test = $album;
			if(length($test) > 256) {
				print "\nComment should not be longer than 256 characters.";
				print "\nPlease enter the album name again! \n";
			}
		}
		# If CDDB entry confirmed, take it
		if(defined $cd{title} && $album eq "") {
			$album = $cd{title};
		}
		# If CDDB entry CHANGED, submission OK
		elsif(defined $cd{title} && $album ne "") {
			$cddbsubmission = 1;
		}
		if($album eq "") {
			$album = "Unknown Album";
			$cddbsubmission = 0;
		}
		$album =~ s/`/'/g;	# Change ` to ' to avoid problems later
		$album =~ s/[;|><"\015]//g;
		$albumtag = $album;
		$album =~ s/[:*#?$\!]//g if($chars == 0);
		$album =~ s/[*]//g;
		$album =~ s/\//-/g;	# Change / to - to avoid problems later
		$dtitle = $artistag . " / " . $albumtag;
		print "\n";
		$i=1;
		while($i <= $tracks) {	# Treat $tracks as scalar
			if(defined $cd{track}[$i-1]) {
				printf("   Track %02d ($tracklist[$i-1]): %s", $i);
			}
			else {
				printf("   Track %02d: %s", $i);
			}
			$track = <STDIN>;
			chomp $track;
			$track =~ s/`/'/g;	# Change ` to ' to avoid problems later
			$track =~ s/[;|><"\015]//g;
			$tracktag = $track;
			$track =~ s/[:*#?$\!]//g if($chars == 0);
			$track =~ s/[*]//g;
			$track =~ s/\//-/g;	# Change / to - to avoid problems later
			if(length($track) > 256) {
				print "\nComment should not be longer than 256 characters.";
				print "\nPlease enter the track name again! \n";
				next;
			}
			# If CDDB entry confirmed, take and replace it in tracklist
			if(defined $cd{track}[$i-1] && $track ne "") {
				splice @tracklist, $i-1, 1, $track;
				splice @tracktags, $i-1, 1, $tracktag;
				$cddbsubmission = 1;
			}
			elsif(!$cd{track}[$i-1] && $track eq "") {
				$track = "Track";
				$cddbsubmission = 0;
			}
			# Fill the "empty" array @{$cd{track}}
			push @{$cd{track}}, "$track";
			$i++;
		}
	}
	else {
		die "You should choose 0 or 1";
	}
	$nocddbinfo = 0;		# Got default/manual CDDB info OK
}
########################################################################
#
# Read the CD and generate a TOC with DiscID, track frames and total
# length. Then prepare CDDB-submission with entries from @tracklist.
#
sub pre_subm {
	my($check,$i,$ans,$date,$line,$oldcat,$subject,$test) = (0,0);
	# Check for CDDB ID vs CD ID problems.
	if($cddbid ne $cd{id} && defined $cd{id}) {
		print "\nWarning: CDID ($cddbid) is not identical to CDDB entry ($cd{id})!";
		print "\nYou might get a collision error. Try anyway!\n";
		$revision = 0
	}
	# Ask if CDDB entries shall be changed and
	# ask if missing entries shall be filled.
	if(defined $cd{year}) {
		$year = get_answ(year,$year);
	}
	if($year eq "") {
		while($year !~ /^\d{4}$| / || !$year ) {
		print "\nPlease enter the year (or none): ";
		$year = <STDIN>;
		chomp $year;
		last if(!$year);
		}
	}
	$cddbsubmission = 1 unless($year eq $cd{year});
	# Ask if CDDB category shall be changed and check if done.
	$oldcat = $categ;
	if(defined $cd{cat}) {
		$categ = get_answ('CDDB category',$categ);
	}
	if($categ eq "") {
		print "\nPlease enter a valid CDDB category";
		print "\nE.g. blues, classical, country, data, folk, jazz, ";
		print "\n     newage, reggae, rock, soundtrack, misc \n";
		while($categ !~ /^blues$|^classical$|^country$|^data$|^folk$|^jazz$|^newage$|^reggae$|^rock$|^soundtrack$|^misc$/ ) {
			print "\nPlease enter a valid CDDB category (rock): ";
			$categ = <STDIN>;
			chomp $categ;
			if($categ eq "") {
				$categ = rock;
			}
		}
		$cddbsubmission = 1 unless($categ eq $cd{cat});
	}
	# If one changes catecory for a new submission, set Revision to 0.
	if($oldcat ne $categ && defined $cd{cat}){
		$revision = 0;
	}
	# Remind the user if genre is not ID3v2 compliant!
	if($coder != 0 && $genre ne "") {
		&check_genre();
		$cddbsubmission = 1 unless($genre eq $cd{'genre'});
	}
	# It would be better not to ask if genre had been passed
	# from command line... but then I need one more var to compare!
	if(defined $cd{'genre'} && $genre ne "") {
		$genre = get_answ(genre,$genre);
	}
	if($genre eq "") {
		print "\nNo valid genre";
		print "\nPlease enter a valid CDDB genre (or none): ";
		$genre = <STDIN>;
		chomp $genre;
		# Allow to submit no genre! Else check it!
		# Set cddbgenre flag to 2.
		if($genre ne "") {
			$genre =~ s/[\015]//g;
			$cddbgenre = 2;
			&check_genre();
		}
	}
	$cddbsubmission = 1 unless($genre eq $cd{'genre'});
	# Now start to write the CDDB submission
	open(TOC, ">$homedir/cddb.toc") || die "Can not write to cddb.toc $!";
	print TOC "# xmcd CD database generated by RipIT\n";
	print TOC "#\n";
	print TOC "# Track frame offsets:\n";
	$i = 0;
	foreach (@framelist) {
		print TOC "# $_\n" if($i < $#framelist);
		$i++;
	}
	print TOC "#\n";
	print TOC "# Disc length: $totals seconds\n";
	print TOC "#\n";
	print TOC "# Revision: $revision\n";
	$date = `date`;
	chomp $date;
	print TOC "# Submitted via: RipIT $version www.suwald.com/ripit/ripit.html at $date\n";
	print TOC "#\n";
	print TOC "DISCID=$cddbid\n";
	print TOC "DTITLE=$dtitle\n";
	print TOC "DYEAR=$year\n";
	if(defined $genre){
		print TOC "DGENRE=$genre\n";
	}
	elsif($genre eq "" && defined $categ) {
		print TOC "DGENRE=$categ\n";
	}
	$i = 0;
	foreach (@tracktags) {
		print TOC "TTITLE$i=$_\n";
		++$i;
	}
	if($commentblock[0] ne "") {
		$ans = x;
		$check = 0;
		print "Confirm (Enter), delete or edit each comment line (c/d/e)!\n";
		foreach (@commentblock) {
			while($ans !~ /^c|^d|^e/) {
				print "$_ (c/d/e): ";
				$ans = <STDIN>;
				chomp $ans;
				if($ans eq "") {
					$ans = c;
				}
			}
			if($ans =~ /^c/ || $ans eq "") {
				print TOC "EXTD=$_\\n\n";
				$check = 1;
			}
			elsif($ans =~ /^e/) {
				$test="a"x257;
				while(length($test) > 256) {
					print "Enter a different line: \n";
					my $ans = <STDIN>;
					chomp $ans;
					$ans =~ s/[;*#|><"\$\015]//g;
					# TODO: split too long lines instead of
					# reentering them!
					$test = $ans;
					if(length($ans) <= 256) {
						print TOC "EXTD=$ans\n";
					}
					else {
						print "\nComment should not be longer than 256 characters.";
						print "\nPlease enter the comment again! \n";
					}
				}
				$cddbsubmission = 1;
				$ans = x;
				$check = 1;
			}
			else {
				# Don't print the line.
				$cddbsubmission = 1;
			}
			$ans = x;
		}
		$line = "a";
		while(defined $line ) {
			print "Do you want to add a line? (Enter for none or type!): ";
			$line = <STDIN>;
			chomp $line;
			$line =~ s/[;*#|><"\$\015]//g;
			$cddbsubmission = 1 if($line ne "");
			last if(!$line);
			# TODO: split too long lines instead of
			# reentering them!
			if(length($line) <= 256) {
				print TOC "EXTD=$line\\n\n";
			}
			else {
				print "\nComment should not be longer than 256 characters.";
				print "\nPlease enter the comment again! \n";
			}
			$check = 1;
		}
		# If all lines have been deleted, add an empty EXTD line!
		if($check == 0){
			print TOC "EXTD=\n";
		}
	}
	# If there are no comments, ask to add some.
	elsif($commentblock[0] eq "") {
		$line = "a";
		while(defined $line ) {
			print "Please enter a comment line (or none): ";
			$line = <STDIN>;
			chomp $line;
			$line =~ s/[;*#|><"\$\015]//g;
			$cddbsubmission = 1 if($line ne "");
			# TODO: split too long lines instead of
			# reentering them!
			if(length($line) <= 256) {
				print TOC "EXTD=$line\n";
			}
			else {
				print "\nComment should not be longer than 256 characters.";
				print "\nPlease enter the comment again! \n";
			}
			# This line has to be written, so break the
			# while loop here and not before, as above.
			last if(!$line);
		}
	}
	else {
		print TOC "EXTD=\n";
	}

	$ans = get_answ('Track comment','existing ones');
	if($ans eq "") {
		$i = 0;
		while($i < $tracks) {	# Treat $tracks as scalar
			$test="a"x257;
			while(length($test) > 256) {
				if($trackcom[$i] ne "") {
					printf("   Track comment %02d ($trackcom[$i]): %s", $i+1);
				}
				else {
					printf("   Track comment %02d: %s", $i+1);
				}
				$track = <STDIN>;
				chomp $track;
				$track =~ s/`/'/g;	# Change ` to ' to avoid problems later
				$track =~ s/[#|\015]//g;
				$test = $track;
				if(length($test) > 256) {
					print "\nComment should not be longer than 256 characters.";
					print "\nPlease enter the track comment again! \n";
				}
			}
			# If CDDB entry confirmed, take and replace it in tracklist
			if(defined $trackcom[$i] && $track eq "") {
				print TOC "EXTT$i=$trackcom[$i]\n";
			}
			elsif(defined $trackcom[$i] && $track ne "") {
				print TOC "EXTT$i=$track\n";
				$cddbsubmission = 1;
			}
			elsif($track ne "") {
				print TOC "EXTT$i=$track\n";
				$cddbsubmission = 1;
			}
			else {
				print TOC "EXTT$i=\n";
			}
			$i++;
		}
	}
	elsif(defined @trackcom) {
		$i = 0;
		foreach (@tracklist) {
			print TOC "EXTT$i=$trackcom[$i]\n";
			++$i;
		}
	}
	else {
		$i = 0;
		foreach (@tracklist) {
			print TOC "EXTT$i=\n";
			++$i;
		}
	}

	print TOC "PLAYORDER=$playorder\n";
	close(TOC);
	# Copy the *edited* CDDB file if variable set to the ~/.cddb/
	# directory.
	if($archive == 1 && $cddbsubmission != 2) {
		system("mkdir -m 0777 -p \"$homedir/.cddb/$categ\"") && \
		die "Cannot create directory \"$homedir/.cddb/$categ\": $!";
		system("cp \"$homedir/cddb.toc\" \"$homedir/.cddb/$categ/$cddbid\"") && \
		die "Cannot copy cddb.toc to directory \"$homedir/.cddb/$categ/$cddbid\": $!";
		print "Saved file $cddbid in \"$homedir/.cddb/$categ/\"";
	}
	print "\n";
	# Don't send mail if nail command unknown or not connected to
	# the internet.
	unless(!system "nail -V") {
		print "nail (mail) not installed?\n";
		$cddbsubmission = 0;
	}
	# If no connection to the internet do not submit.
	if($submission == 0) {
		$cddbsubmission = 0;
	}
	if($cddbsubmission == 1) {
		while($mailad !~ /.@.+[.]./) {
			print "\nReady for submission, enter a valid return e-mail address: ";
			$mailad = <STDIN>;
			chomp $mailad;
		}
		$subject = "cddb " . $categ . " " . $cddbid;
		open TOC, "cat \"$homedir/cddb.toc\" |" or die "Can not open file cddb.toc $!";
		open MAIL, "|nail freedb-submit\@freedb.org -r $mailad -s \"$subject\" " or die "nail not installed? $!";
		my @lines = <TOC>;
		foreach (@lines) {
			print MAIL "$_";
		}
		close MAIL;
		die "Mail exit status not zero: $?" if $?;
		close TOC;
		print "CDDB entry submitted\n\n";
		unlink("$homedir/cddb.toc");
	}
	elsif($cddbsubmission == 2) {
		print "\nCDDB entry created but not send because no changes";
		print "\nwere made! Please edit and send it manually to ";
		print "\nfreedb-submit\@freedb.org with Subject:";
		print "\ncddb $categ $cddbid\n\n";
	}
	else {
		print "\nCDDB entry saved in your home directory, but not send,";
		print "\nplease edit it and send it manually to:";
		print "\nfreedb-submit\@freedb.org  with Subject:";
		print "\ncddb $categ $cddbid\n\n";
	}
}
########################################################################
#
# Check if genre is correct
#

sub check_genre {
   my $genre = $_[0];
   my $genreno = "";
   my $genrenoflag = 1;

   $genre = "  " if($genre eq "");

   # If Lame is not used, don't die if ID3v2-tag is not compliant.
   if($lameflag == 0) {
      unless(log_system(
         "lame --genre-list 2>&1 | grep -i \" $genre\$\" > /dev/null ")) {
         print "Genre $genre is not ID3v2 compliant!\n"
            if($verbose >= 1);
         print "Continuing anyway!\n\n" if($verbose >= 1);
         chomp($genreno = "not ID3v2 compliant!\n");
      }
      return ($genre,$genreno);
   }

   # If Lame is not installed, don't loop for ever.
   if($lameflag == -1) {
      chomp($genreno = "Unknown.\n");
      return ($genre,$genreno);
   }

   # Check if (similar) genre exists. Enter a new one with interaction,
   # or take the default one.
   while(!log_system(
      "lame --genre-list 2>&1 | grep -i \"$genre\" > /dev/null ")) {
      print "Genre $genre is not ID3v2 compliant!\n" if($verbose >= 1);
      if($interaction == 1) {
         print "Use \"lame --genre-list\" to get a list!\n";
         print "\nPlease enter a valid CDDB genre (or none): ";
         $genre = <STDIN>;
         chomp $genre;
         $cd{genre} = $genre;
      }
      else {
         print "Genre \"Other\" will be used instead!\n"
            if($verbose >= 1);
         $genre = "12 Other";
      }
   }

   if($genre eq "") {
      return;
   }
   elsif($genre =~ /^\d+$/) {
      chomp($genre = `lame --genre-list 2>&1 | grep -i \' $genre \'`);
   }
   else {
      # First we want to be sure that the genre from the DB, which might
      # be "wrong", e.g. wave (instead of Darkwave or New Wave) or synth
      # instead of Synthpop, will be correct. Put the DB genre to ogenre
      # and get a new right-spelled genre... Note, we might get several
      # possibilities, e.g. genre is Pop, then we get a bunch of
      # "pop-like" genres!
      # There will be a line break, if multiple possibilities found.
      my $ogenre = $genre;
      chomp($genre = `lame --genre-list 2>&1 | grep -i \'$genre\'`);
      # Second we want THE original genre, if it precisely exists.
      chomp(my $testgenre = `lame --genre-list 2>&1 | grep -i \'\^... $ogenre\$\'`);
      $genre = $testgenre if($testgenre);
      # If we still have several genres:
      # Either let the operator choose, or if no interaction, take
      # default genre: "12 Other".
      if($genre =~ /\n/ && $interaction == 1) {
         print "More than one genre possibility found!\n";
         my @list = split(/\n/,$genre);
         my ($i,$j) = (0,1);
         while($i > $#list+1 || $i == 0) {
            # TODO: Here we should add the possibility to choose none!
            # Or perhaps to go back and choose something completely
            # different.
            foreach (@list) {
               printf(" %2d: $_ \n",$j);
               $j++;
            }
            $j--;
            print "\nChoose [1-$j]: ";
            $i = <STDIN>;
            chomp $i;
            $j = 1;
         }
         chomp($genre = $list[$i-1]);
      }
      # OK, no interaction! Take the first or default genre!
      elsif($genre =~ /\n/ && $interaction != 1 && $lameflag == 1) {
         $genre = "12 Other" if($genre eq "");
         $genre =~ s/\n.*//;
      }
      # OK, the genre is not Lame compliant, and we do not care about,
      # because Lame is not used. Set the genre-number-flag to 0 to
      # prevent genre-number-extracting at the end of the subroutine.
      elsif($lameflag != 1) {
         $genre = $ogenre;
         $genrenoflag = 0;
      }
      chomp $genre;
   }

   # Extract genre-number.
   if($genre ne "" && $genrenoflag == 1) {
      $genre =~ s/^\s*//;
      my @genre = split(/ /, $genre);
      $genreno = shift(@genre);
      $genre = "@genre";
   }
   return ($genre,$genreno);
}

#sub check_genre {
#	my ($i,$j) = (0,1);
#
#	# If Lame is not used, don't die if ID3v2-tag is not compliant.
#	if($lameflag == 0) {
#		if(system "lame --genre-list | grep -i \" $genre\" > /dev/null " ) {
#			print "Genre $genre is not ID3v2 compliant!\n" if($quiet == 0);
#			print "I continue anyway!\n\n" if($quiet == 0);
#			$genreno = "not ID3v2 compliant!\n";
#			chomp $genreno;
#			return;
#		}
#	}
#	# Now check if genre is OK... if not, delete it!
#	while(system "lame --genre-list | grep -i \"$genre\" > /dev/null " ) {
#		if($quiet == 0) {
#			print "Genre $genre is not ID3v2 compliant!\n";
#			print "\nPlease enter a valid CDDB genre (or none): ";
#			$genre = <STDIN>;
#			chomp $genre;
#		}
#		else {
#			$genre = "Other";
#		}
#		# If $genre was from CDDB set the $cddbgenre flag to 0.
#		if($cddbgenre == 1) {
#			$cddbgenre = 0;
#		}
#	}
#	# ...and if none is choosen return to "get CDDB" or "CDDB
#	# submission"  ($cddbgenre is 0, 1 or 2)...
#	if($genre eq "" && $cddbgenre != 1) {
#		return;
#	}
#	# ...or if genre is OK (from above) do quick check if it came
#	# from "get CDDB" subroutine...
#	elsif($cddbgenre == 1 && $genre ne "") {
#		$genre = `lame --genre-list | grep -i \'\^... $genre\$\' `;
#		chomp $genre;
#		$cddbgenre = 0;
#	}
#	# ... or if genre was passed as number...
#	elsif($genre =~ /^\d/) {
#		if($genre < 10) {
#			$genre = `lame --genre-list | grep -i \"  $genre\" `;
#		}
#		elsif($genre =~ /^\d{2}$/) {
#			$genre = `lame --genre-list | grep -i \" $genre\" `;
#		}
#		else {
#			$genre = `lame --genre-list | grep -i \"$genre\" `;
#		}
#		chomp $genre;
#	}
#	# ... or if genre was passed as word.
#	elsif($genre =~ /\w/) {
#		$genre = `lame --genre-list | grep -i \" $genre\" `;
#		chomp $genre;
#		if($genre =~ /\n/) {
#			print "More than one genre possibility found!\n";
#			my @list = split(/\n/,$genre);
#			while($i > $#list+1 || $i == 0) {
#				foreach (@list) {
#					printf(" %2d: $_ \n",$j);
#					$j++;
#				}
#				$j--;
#				print "\nChoose [1-$j]: ";
#				$i = <STDIN>;
#				chomp $i;
#				$j = 1;
#			}
#			$genre = $list[$i-1];
#			chomp $genre;
#		}
#	}
#	elsif($cddbgenre != 2) {
#		die "Genre $genre is invalid!";
#	}
#	@genre = split(/ /, $genre);
#	while($genre[0] eq "") {
#		$genreno = shift(@genre);
#	}
#	$genreno = shift(@genre);
#	$genre = "@genre";
#}
########################################################################
#
# Check mirrors
#
sub check_host {
	while($mirror !~ /^freedb$|^at$|^au$|^ca$|^ca2$|^de$|^es$|^fi$|^ru$|^uk$|^us$/){
		print "host mirror ($mirror) not valid!\n";
		print "enter freedb, at, au, ca, ca2, de, es, fi, ru, uk, or us: ";
		$mirror = <STDIN>;
		chomp $mirror;
	}
	$CDDB_HOST = $mirror.".freedb.org";
	chomp $CDDB_HOST;
}
########################################################################
#
# Reply to question
#
sub get_answ {
	my $ans = x;
	while($ans !~ /^y|^n/) {
		print "Do you want to enter a different ".$_[0]." than ".$_[1]."? [y/n], (n): ";
		$ans = <STDIN>;
		chomp $ans;
		if($ans eq ""){
			$ans = n;
		}
	}
	if($ans =~ /^y/){
		return "";
	}
	return $_[1];
}
########################################################################
#
# Check quality passed from command line for oggenc, flac and lame only
# if vbr wanted (then it's encoder no 3).
#
sub check_quality {
	my $corrflag = 0;
	if($qualame ne "off") {
		while($qualame > 9) {
			print "\nThe quality $qualame is not valid for Lame!\n";
			print "Please enter a different quality (0 = best), [0-9]: ";
			$qualame = <STDIN>;
			chomp $qualame;
			$corrflag = 1;
		}
	}
	if($qualogg ne "off") {
		while($qualogg > 10 || $qualogg == 0) {
			print "\nThe quality $qualogg is not valid for Ogg!\n";
			print "Please enter a different quality (10 = best), [1-10]: ";
			$qualogg = <STDIN>;
			chomp $qualogg;
			$corrflag = 1;
		}
	}
	if($quaflac ne "off") {
		while($quaflac > 8) {
			print "\nThe compression level $quaflac is not valid for Flac!\n";
			print "Please enter a different compression level (0 = lowest), [0-8]: ";
			$quaflac = <STDIN>;
			chomp $quaflac;
			$corrflag = 1;
		}
	}
	if($quafaac ne "off") {
		while($quafaac > 500 || $quafaac < 10) {
			print "\nThe quality $quaflac is not valid for Faac!\n";
			print "Please enter a different quality (500 = max), [10-500]: ";
			$quafaac = <STDIN>;
			chomp $quafaac;
			$corrflag = 1;
		}
	}
	# Now save the corrected values to the pquality array! Do it in
	# the same order the encoders had been passed! If no encoders
	# were passed, test if there are encoders in the config file...
	if($corrflag == 1) {
		@pquality = split(/,/, join(',', @pquality));
		for($index = 0; $index <= $#pquality; $index++) {
			if($coder[$index] =~ /^\d/) {
				$pquality[$index] = $qualame if($coder[$index] == 0);
				$pquality[$index] = $qualogg if($coder[$index] == 1);
				$pquality[$index] = $quaflac if($coder[$index] == 2);
				$pquality[$index] = $quafaac if($coder[$index] == 3);
			}
			else {
				$pquality[$index] = $qualame if($index == 0);
				$pquality[$index] = $qualogg if($index == 1);
				$pquality[$index] = $quaflac if($index == 2);
				$pquality[$index] = $quafaac if($index == 3);
			}
		}
		$pquality = join(',',@pquality);
		@pquality = ();
		$pquality[0] = $pquality;
	}
}
########################################################################
#
# Check bitrate for Lame only if vbr is wanted.
#
sub check_vbrmode {
	while($vbrmode ne "new" && $vbrmode ne "old") {
		print "\nFor vbr using Lame choose *new* or *old*! (new): ";
		$vbrmode = <STDIN>;
		chomp $vbrmode;
		$vbrmode = new if ($vbrmode eq "");
	}
}
########################################################################
#
# Check preset for Lame only.
#
sub lame_preset {
	if($vbrmode eq "new") {
		$preset = "fast " . $preset;
	}
}
########################################################################
#
# Check if there is an other than $cddevice which has a CD if no --dev
# option was given.
#
sub check_cddev {
	if(open(CD, "$cddevice")) {
		$cddev = $cddevice;
		close CD;
	}
	else {
		# Remember: match character "ranges" in []-brackets,
		# grep '^/dev/[^fd,^hd,^sd]' is not what we want!
		open(DEV, "grep '^/dev/[^f,^h]' /etc/fstab | awk '{print \$1}' |");
		@dev = <DEV>;
		@dev = grep(!/sd/, @dev);
		close DEV;
		foreach (@dev) {
			if(open(CD, "$_")) {
				$cddev = $_;
				chop $cddev;
				close CD;
			}
		}
	}
}
########################################################################
#
# Check bitrate if bitrate is not zero.
#
sub check_bitrate {
	while($bitrate !~ /^32$|^40$|^48$|^56$|^64$|^80$|^96$|^112$|^128$|^160$|^192$|^224$|^256$|^320$|^off$/) {
		print "\nBitrate should be one of the following numbers or \"off\"! Please Enter";
		print "\n32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256 or 320: (128) \n";
		$bitrate = <STDIN>;
		chomp $bitrate;
		if($bitrate eq "") {
			$bitrate = 128;
		}
	}
}
########################################################################
#
# Check protocol level for CDDB query.
#
sub check_proto {
	while($proto > 6) {
		print "Protocol level for CDDB query should be less-equal 6!\n";
		print "Enter an other value for protocol level (6): ";
		$proto =  <STDIN>;
		chomp $proto;
		$proto = 6 if($proto eq "");
	}
}
########################################################################
#
# Check, clean and sort the coder array, and then, according to, the
# qualities.
#
sub check_coder {
	# Create encoder array if passed or read from config file.
	if($pcoder[0] =~ /^\d/) {
		@coder = split(/,/, join(',',@pcoder));
	}
	else {
		@coder = split /,/, join(',', @coder);
	}
	# Now check if an encoder was passed several times.
	@scoder = sort {$a <=> $b} @coder;
	my $index=0;
	while($index < $#scoder) {
		if($scoder[$index] == $scoder[$index+1]) {
			# TODO delete entry if encoder appears more than once.
#			splice @coder,$index,1;
			die "Found encoder $scoder[$index] twice, confused!\n";
		}
		$index++;
	}
	# Now check, that there is no entry >= 4, find the index
	# of lame if present and calculate the number of encoders
	# that need a quality entry.
	my $nofcoders=0;
	for($index = 0; $index <= $#coder; $index++) {
		if($coder[$index] >= 4) {
			print "Encoder number $coder[$index] does not yet exist, please enter\n";
			die "0 for Lame, 1 for Oggenc, 2 for Flac and 3 for Faac!\n\n";
		}
		$nofcoders++;
		if($coder[$index] == 0 || $coder[$index] >= 4) {
			$lameindex = $index;
			$nofcoders = $nofcoders - 1;
			$lameflag = 1;
		}
	}
	# Use comma separated string to write the encoder array to the
	# config file!
	$wcoder = join(',',@coder);
	$nofcoders=$nofcoders-1;
	# Now order the qualities (if passed) in the same way as coders!
	# Note, the array of coders is NOT sorted!
	# First, check if there is a config file with an probably unusual
	# order of encoders. In this way, the check-quality subroutine
	# will ask the correct questions and not mess up the encoders!
	if($pcoder[0] !~ /^\d/ && -r "$homedir/.ripit/config") {
		open(CONF, "$homedir/.ripit/config");
		my @conflines = <CONF>;
		close CONF;
		@pcoder = grep(s/^coder=//, @conflines) if($pcoder[0] !~ /^\d/);
		chomp @pcoder;
		if($pcoder[0] =~ /^\d/) {
			@coder = split(/,/, join(',',@pcoder));
		}
	}
	# Second, check if quality was passed.
	if($pquality[0] =~ /^\d/ || $pquality[0] eq "off") {
		@quality = split(/,/, join(',', @pquality));
		# If lame was passed, but no quality for lame (because
		# user wants cbr), then add the default quality for lame
		# at the right place of array @quality to ensure that the
		# &check_quality() subroutine works! NOTE: $nofcoder is
		# the number of non-lame encoders!
		if($#quality == $nofcoders && $lameflag == 1) {
			splice @quality, $lameindex, 1, (5,$quality[$lameindex]);
		}
		for($index = 0; $index <= $#quality; $index++) {
			if($coder[$index] =~ /^\d/) {
				$qualame = $quality[$index] if($coder[$index] == 0);
				$qualogg = $quality[$index] if($coder[$index] == 1);
				$quaflac = $quality[$index] if($coder[$index] == 2);
				$quafaac = $quality[$index] if($coder[$index] == 3);
			}
			else {
				$qualame = $quality[$index] if($index == 0);
				$qualogg = $quality[$index] if($index == 1);
				$quaflac = $quality[$index] if($index == 2);
				$quafaac = $quality[$index] if($index == 3);
			}
		}
	}
}
########################################################################
#
# Over or re-write the config file (depends on option savenew or save).
#
sub save_config {
	system("mkdir -m 0777 -p ~/.ripit") && \
	die "Cannot create directory ~/.ripit/config: $!";
	my $ripdir = $homedir . "/.ripit";
	rename("$ripdir/config","$ripdir/config.old") if(-r "$ripdir/config");
	open(CONF, "> $ripdir/config") || die "Can not write to ~/.ripit/config: $!";
	print CONF "archive=$archive\n";
	print CONF "bitrate=$bitrate\n";
	print CONF "maxrate=$maxrate\n";
	print CONF "cddevice=$cddev\n"; # Don't use $device here!
	print CONF "CDDBHOST=$DCDDB_HOST\n";
	print CONF "chars=$chars\n";
	print CONF "coder=$wcoder\n"; # $pcoder was checked!
	print CONF "comment=$commentag\n";
	print CONF "eject=$eject\n";
	print CONF "encode=$encode\n";
	print CONF "faacopt=$faacopt\n";
	print CONF "flacopt=$flacopt\n";
	print CONF "lameopt=$lameopt\n";
	print CONF "oggopt=$oggopt\n";
	print CONF "halt=$halt\n";
	print CONF "interaction=$interaction\n";
	print CONF "lcd=$lcd\n";
	print CONF "lcdhost=$lcdhost\n";
	print CONF "lcdport=$lcdport\n";
	print CONF "local=$local\n";
	print CONF "lowercase=$lowercase\n";
	print CONF "mirror=$mirror\n";
	print CONF "mailad=$mailad\n";
	print CONF "nice=$nice\n";
	print CONF "output=$outputdir\n";
	print CONF "paranoia=$parano\n";
	print CONF "playlist=$playlist\n";
	print CONF "preset=$wpreset\n";
	print CONF "proto=$proto\n";
	print CONF "proxy=$proxy\n";
	print CONF "quafaac=$quafaac\n";
	print CONF "quaflac=$quaflac\n";
	print CONF "qualame=$qualame\n";
	print CONF "qualogg=$qualogg\n";
	print CONF "quiet=$quiet\n";
	print CONF "ripper=$ripper\n";
	print CONF "ripopt=$ripopt\n";
	print CONF "scp=$scp\n";
	print CONF "sshlist=$wsshlist\n";
	print CONF "submission=$submission\n";
	print CONF "transfer=$transfer\n";
	print CONF "underscore=$underscore\n";
	print CONF "utftag=$utftag\n";
	print CONF "vbrmode=$vbrmode\n";
	print CONF "year=$year\n";
	print CONF "wav=$wav\n";
	print CONF "dirtemplate=$dirtemplate\n";
	print CONF "tracktemplate=$tracktemplate\n";
	close CONF;
}
########################################################################
#
# Read the config file, take the parameters only if NOT yet defined!
#
sub read_config {
	my $ripdir = $homedir."/.ripit/config";
	if(-r "$ripdir" ) {
		open(CONF, "$ripdir") || print "Can not read config file!\n";
		my @conflines = <CONF>;
		close CONF;
		my @archive = grep(s/^archive=//, @conflines);
		$archive = @archive[0] if($parchive !~ /^\d/);
		chomp $archive;
		my @bitrate = grep(s/^bitrate=//, @conflines);
		$bitrate = @bitrate[0] if(!$pbitrate);
		chomp $bitrate;
		my @maxrate = grep(s/^maxrate=//, @conflines);
		$maxrate = @maxrate[0] if(!$pmaxrate);
		chomp $maxrate;
		my @cddev = grep(s/^cddevice=//, @conflines);
		$cddev = @cddev[0] if(!$pcddev);
		chomp $cddev;
		my @chars = grep(s/^chars=//, @conflines) if($pchars !~ /^\d/);
		$chars = @chars[0] if(@chars[0]);
		chomp $chars;
		my @commentag = grep(s/^comment=//, @conflines);
		$commentag = @commentag[0] if(!$pcommentag);
		chomp $commentag;
		my @CDDB_HOST = grep(s/^CDDBHOST=//, @conflines);
		$CDDB_HOST = @CDDB_HOST[0] if(!$PCDDB_HOST);
		chomp $CDDB_HOST;
		@pcoder = grep(s/^coder=//, @conflines) if($pcoder[0] !~ /^\d/);
		chomp @pcoder;
		# NOTE: all coders are in array entry $pcoder[0]!
		my @dirtemplate = grep(s/^dirtemplate=//, @conflines);
		$dirtemplate = @dirtemplate[0] if(!$pdirtemplate);
		chomp $dirtemplate;
		my @eject = grep(s/^eject=//, @conflines);
		$eject = @eject[0] if($peject !~ /^\d/);
		chomp $eject;
		my @encode = grep(s/^encode=//, @conflines);
		$encode = @encode[0] if($pencode !~ /^\d/);
		chomp $encode;
		my @halt = grep(s/^halt=//, @conflines);
		$halt = @halt[0] if(!$phalt);
		chomp $halt;
		my @interaction = grep(s/^interaction=//, @conflines);
		$interaction = @interaction[0] if($pinteraction !~ /^\d/);
		chomp $interaction;
		my @lcd = grep(s/^lcd=//, @conflines);
		$lcd = @lcd[0] if($plcd !~ /^\d/);
		chomp $lcd;
		my @lcdhost = grep(s/^lcdhost=//, @conflines);
		$lcdhost = @lcdhost[0] if(!$plcdhost);
		chomp $lcdhost;
		my @lcdport = grep(s/^lcdport=//, @conflines);
		$lcdport = @lcdport[0] if(!$plcdport);
		chomp $lcdport;
		my @local = grep(s/^local=//, @conflines);
		$local = @local[0] if($plocal !~ /^\d/);
		chomp $local;
		my @lowercase = grep(s/^lowercase=//, @conflines);
		$lowercase = @lowercase[0] if($plowercase !~ /^\d/);
		chomp $lowercase;
		my @mailad = grep(s/^mailad=//, @conflines);
		$mailad = @mailad[0] if(!$pmailad);
		chomp $mailad;
		my @mirror = grep(s/^mirror=//, @conflines);
		$mirror = @mirror[0] if(!$pmirror);
		chomp $mirror;
		my @nice = grep(s/^nice=//, @conflines);
		$nice = @nice[0] if($pnice !~ /^\d/);
		chomp $nice;
		my @outputdir = grep(s/^output=//, @conflines);
		$outputdir = @outputdir[0] if(!$poutputdir);
		chomp $outputdir;
		my @parano = grep(s/^paranoia=//, @conflines);
		$parano = @parano[0] if($pparano !~ /^\d/);
		chomp $parano;
		my @playlist = grep(s/^playlist=//, @conflines);
		$playlist = @playlist[0] if($pplaylist !~ /^\d/);
		chomp $playlist;
		my @preset = grep(s/^preset=//, @conflines);
		$preset = @preset[0] if(!$ppreset);
		chomp $preset;
		my @proto = grep(s/^proto=//, @conflines);
		$proto = @proto[0] if(!$pproto);
		chomp $proto;
		my @proxy = grep(s/^proxy=//, @conflines);
		$proxy = @proxy[0] if(!$pproxy);
		chomp $proxy;
		my @quafaac = grep(s/^quafaac=//, @conflines) if($pquality[0] !~ /^\d/);
		$quafaac = @quafaac[0] if(@pquafaac[0] !~ /^\d$|^off/);
		chomp $quafaac;
		my @quaflac = grep(s/^quaflac=//, @conflines) if($pquality[0] !~ /^\d/);
		$quaflac = @quaflac[0] if($pquality[0] !~ /^\d$|^off/);
		chomp $quaflac;
		my @qualame = grep(s/^qualame=//, @conflines) if($pquality[0] !~ /^\d/);
		$qualame = @qualame[0] if($pquality[0] !~ /^\d$|^off/);
		chomp $qualame;
		my @qualogg = grep(s/^qualogg=//, @conflines) if($pquality[0] !~ /^\d/);
		$qualogg = @qualogg[0] if($pquality[0] !~ /^\d$|^off/);
		chomp $qualogg;
		my @faacopt = grep(s/^faacopt=//, @conflines);
		$faacopt = @faacopt[0] if(!$pfaacopt);
		chomp $faacopt;
		my @flacopt = grep(s/^flacopt=//, @conflines);
		$flacopt = @flacopt[0] if(!$pflacopt);
		chomp $flacopt;
		my @lameopt = grep(s/^lameopt=//, @conflines);
		$lameopt = @lameopt[0] if(!$plameopt);
		chomp $lameopt;
		my @oggopt = grep(s/^oggopt=//, @conflines);
		$oggopt = @oggopt[0] if(!$poggopt);
		chomp $oggopt;
		my @quiet = grep(s/^quiet=//, @conflines);
		$quiet = @quiet[0] if($pquiet !~ /^\d/);
		chomp $quiet;
		my @ripper = grep(s/^ripper=//, @conflines);
		$ripper = @ripper[0] if($pripper !~ /^\d/);
		chomp $ripper;
		my @ripopt = grep(s/^ripopt=//, @conflines);
		$ripopt = @ripopt[0] if($pripopt !~ /^\d/);
		chomp $ripopt;
		my @rlist = grep(s/^sshlist=//, @conflines) if(!$psshlist[0]);
		chomp @rlist;
		# NOTE: all machine names are in array entry $rlist[0]!
		@sshlist = split(/,/, join(',',@rlist));
		my @scp = grep(s/^scp=//, @conflines);
		$scp = @scp[0] if($pscp !~ /^\d/);
		chomp $scp;
		my @submission = grep(s/^submission=//, @conflines);
		$submission = @submission[0] if($psubmission !~ /^\d/);
		chomp $submission;
		my @transfer = grep(s/^transfer=//, @conflines);
		$transfer = @transfer[0] if(!$ptransfer);
		chomp $transfer;
		my @tracktemplate = grep(s/^tracktemplate=//, @conflines);
		$tracktemplate = @tracktemplate[0] if(!$ptracktemplate);
		chomp $tracktemplate;
		my @underscore = grep(s/^underscore=//, @conflines);
		$underscore = @underscore[0] if($punderscore !~ /^\d/);
		chomp $underscore;
		my @utftag = grep(s/^utftag=//, @conflines);
		$utftag = @utftag[0] if($putftag !~ /^\d/);
		chomp $utftag;
		my @vbrmode = grep(s/^vbrmode=//, @conflines);
		$vbrmode = @vbrmode[0] if(!$pvbrmode);
		chomp $vbrmode;
		my @year = grep(s/^year=//, @conflines);
		$year = @year[0] if(!$pyear);
		chomp $year;
		my @wav = grep(s/^wav=//, @conflines);
		$wav = @wav[0] if($pwav !~ /^\d/);
		chomp $wav;
	}
	else {
		print "No config file found!\n" if($quiet == 0);
	}
}

########################################################################
#
# Display a help page and exit!
#
sub print_help {
	print "\n
NAME:
       ripit - Perl Script to Create .flac .ogg .mp3 or .m4a (aac) files
       from an audio CD.
\n
SYNOPSIS:
	ripit [options]
\n
DESCRIPTION:
       This Perl script makes it a lot easier to create \"mp3\" files from
       an audio CD. RipIT supports the formats Flac, Lame, Ogg and Faac.
       It tries to find the artist and song titles with the CDDB_get.pm.
       One can submit and edit CDDB entries \@ freedb.org.
\n
OPTIONS:
       [track_selection]
              Tracks to rip from.  If not specified,  all tracks will be
              ripped.  Specify a single track by using a single  number,
              or a selection of tracks using numbers separated by commas
              or hyphens, i.e. 2,6,10, or 3,5,7-9 .
              Using a number followed by a comma or hyphen will rip from
              that track to the end of the CD i.e. 3,5- rips track 3 and
              from track 5 to the last one.

       -o dir
       --outputdir dir
              Where the sound should go, default: not set.

       -d cddevice
       --device cddevice
              Path of audio CD device, default: /dev/cdrom.

       -r  ripper
       --ripper ripper
              CDripper to use, 0 - dagrab, 1 - cdparanoia, 2 - cdda2wav,
              3 - tosha, 4 - cdd, default cdparanoia. Because cdparanoia
              and dagrab are the only rippers that  immediately fail  on
              data tracks, RipIT can create an error.log if problems are
              encountered and continues to ripp and code without endless
              looping!  Please use  dagrab  or  cdparanoia except if you
              know what you're doing (know that you will wait)! Default:
              1.

       --ripopt
              User definable options for CDripper.

       -Z
       --disable-paranoia
              When using dagrab, the number of retries will be set to 3,
              with cdparanoia this option is equal to the -Z option.
              Default: off.

       -c encoder
       --coder encoder
              Encoder(s) to use, 0 - Lame, 1 - Ogg, 2 - Flac,  3 - Faac,
              a comma seperated list!
              E.g.  --coder 2,0,1,3 --quality 3,5,300   sets compression
              level for Flac to 3,  Ogg-quality to 5 and Faac quality to
              300.  What about Lame-quality?  Lame does not need quality
              for constant-bitrate,  it assumes that no quality is used,
              but sets it to the lame-default value of 5 in the case one
              uses  vbr-mode! Default: 0.
              Better use --coder 2,0,1,3 --quality 3,4,5,300.

       --faacopt Faac-options
              Pass other options to the encoder,  quote them with double
              quotes if needed; default: not set.

       --flacopt Flac-options
              Pass other options to the encoder,  quote them with double
              quotes if needed; default: not set.

       --lameopt Lame-options
              Pass other options to the encoder,  quote them with double
              quotes if needed; default: not set.

       --oggopt Oggenc-options
              Pass other options to the encoder,  quote them with double
              quotes if needed; default: not set.

       -q quality
       --quality quality
             A comma seperated list of values or the word \"off\",  passed
             in the same order as the list of encoders!  If no  encoders
             passed, follow the order of the config file!
             Quality for ogg (1-10), highest = 10;  or compression level
             for Flac (0-8), lowest compression = 0; or quality for Lame
             in vbr mode (0-9),  best quality = 0;  or quality for  Faac
             (10-500), highest = 500;  default: 5,3,5,100.
             The value \"off\" turns option quality off.

       -v mode
       --vbrmode mode
              Variable bitrate, only used with Lame, mode is new or old,
              see Lame manpage.  The Lame-option quality will be changed
              to -V instead of -q if vbr-mode is used; default: not set.

       -b  rate
       --bitrate rate
              Encode \"mp3\" at this bitrate for Lame, if option --vbrmode
              used, bitrate is equal to the -b option, so one might want
              set off, default: 128.

       -B rate
       --maxrate rate
              maxrate (Bitrate) for Lame using --vbrmode is equal to the
              -B option in Lame or the -M option in Oggenc, default: 0.

       -S mode
       --preset mode
              Use the preset switch when encoding with Lame. With otpion
              --vbrmode new --preset fast will be used.  Mode is one of:
              insane (320 kbps @ CBR), extreme (256 kbps), standard (192
              kbps) or medium (160 kbps) or any other valid bitrate.
              Default: off.

       -W
       --chars
              Use special characters in file names; default: on.

       --comment id3 comment
              Specify a comment tag, default: not set.

       -g genre
       --genre genre
              Overrides CDDB genre,  must be a valid Lame-genre name  if
              using Lame, can (but shouldn't) be anything if using other
              encoders, default: not set.

       -y year
       --year year
              Tag sound files with this year, default: not set.

       --sshlist list
              Comma seperated list of remote machines where RipIT should
              encode. The output path must be the same for all machines.
              Specify the login (login\@machine) only if not the same for
              the remote machine. Else just state the machine names. See
              EXAMPLES for more informtation, default: off.

       --scp  If the fs can not be accessed on the remote machines, copy
              the wavs to the remote machines, default: off.

       --local
              Only used with option --sshlist; if all encodings shall be
              done on remote machines, use --nolocal, default: on.

       -t mode
       --transfer mode
              Transfer mode, cddb or http, will set default port to 8880
              or 80 (for http), default: cddb.

       -C server
       --cddbserver server
              CDDB server, default freedb.org. Note, the full address is
              \"mirror\".freedb.org, i. e. default is freedb.freedb.org.

       -m mirror
       --mirror mirror
              Choose \"freedb\" or one of the possible freedb bmirrors, e.
              g. \"at\", default: freedb.  For more  information check the
              webpage  www.freedb.org.

       -L level
       --protocol level
              CDDB protocol level for CDDB query. Level 6 supports UTF-8
              and level 5 not. Use level 5 to suppress UTF-8. Cf. option
              --utftag below. Default: 6

       -P address
       --proxy address
              The http proxy to use when accessing the cddb server.  The
              protocol must be http! Default: not set.

       -M address
       --mail address
              Users return email address, needed for submitting an entry
              to freedb.org; default: not set.

       -D '\"...\$parameters...\"'
       --dirtemplate '\"...\$parameters...\"'
              Use single AND double quotes to pass the parameters of the
              templates!  Allowed  are  following  parameters:   \$album,
              \$artist,  \$genre,  \$trackname,  \$tracknum and \$year, e. g.
              '\"\$artist - \$year\"'.
              Default dirtemplate: '\"\$artist - \$album\"' ,
              default tracktemplate: '\"\$tracknum \$trackname\"' .

       -T '\"...\$parameters...\"'
       --tracktemplate '\"...\$parameters...\"'
              See above.

       -n value
       --nice value
              Set niceness of encoding process, default 0.

       -a
       --archive
              Read and save CDDB files in  ~/.cddb/\"category\" directory,
              where the \"category\" is one of the 11 CDDB categories.
              Default: off.

       -e
       --eject
              Ejects the CD when finished, if your hardware supports it;
              default off.

       --halt
              Powers off the machine when finished if your configuration
              supports it; default: off.

       -s
       --submission
              Specify --nosubmission if the computer is  offline and the
              created file cddb.toc shall be saved in the home directory
              instead of being submitted. With option  --archive it will
              also be saved in the ~/.cddb directory. Note: it is really
              easy to resubmit incomplete CDDB entries!  One can confirm
              each existing field with Enter and add a missing  genre or
              year.  The purpose of this option is to permit the user to
              edit the CDDB data for the own tags,  but not to overwrite
              the original CDDB entry! Default: off.

       -p
       --playlist
              Create the m3u playlist file, or use --noplaylist. Default
              on.

       -i
       --interaction
              Specify --nointeraction if ripit shall take the first CDDB
              entry found and rip without any questioning. Default: on.

      --lcd
              Use lcdproc to display status, default: not set.

      --lcdhost
              Specify the lcdproc host, default: localhost.

      --lcdport
              Specify the lcdport, default: default: 13666.

      -l
      --lowercase
              Lowercase filenames, default: off.

      --quiet
              Do not output any comments, default: off.

       -u
       --underscore
              Use underscores _ instead of spaces in filenames, default:
              off.

       -U
       --utftag
              Decode tags (but not filenames) from UTF-8 to ISO8859-1 if
              (mp3) player doesn't support them in UTF-8, default: off.

       --encode
              Do encode the wavs. Prevent encoding if only the wav-files
              shall be created (then use option --wav, see below).
              Default: on, else use --noencode.

       -w
       --wav
              Keep the wav files after encoding instead of deleting them
              default: off.

       -h
       --help Print this and exit.

       -V
       --version
              Print version and exit.

       --config
              Read parameters from config file or specify  --noconfig to
              prevent reading it; default: on.

       --save Add parameters passed on command line to config file. This
              options does not  overwrite other  settings.  An  existing
              config file will be saved as config.old. Default: off.

       --savenew
              Save all parameters passed on command line to a new config
              file, backup an existing file to config.old. Default: off.


\n
EXAMPLES
       To specify a CD device, type

              ripit --device /dev/sr1

       To specify the output directory, type

              ripit --outputdir /foo/paths/

       To rip'n'code a special track selection, type

              ripit 1,3-6,8-11

       To use several encoders in the same run, type

              ripit --coder 1,0,2 --quality 3,6

       Note:  in this case, RipIT assumes that  Lame (encoder 0) is used
       in CBR mode wihtout quality option, so  Ogg (encoder 1)  will get
       quality 3 and Flac (encoder 2) quality 6.

       To use Lame with variable bitrate (VBR), type

              ripit --vbrmode new --bitrate 0

       Note, one should reset the --bitrate to 0 (zero) if the -b option
       of Lame is not desired. According to VBR mode in Lame, use  \"new\"
       or \"old\".  It is recommended to use the preset switches for Lame,
       (see Lame-man-page) and specify fast encoding with --vbrmode new

             ripit --preset extreme --vbrmode new

       To define a directory template  where the sound files should  go,
       type

              ripit --dirtemplate '\"\$artist - \$year\"'

       To save a config file in ~/home/.ripit/ with options: to use Lame
       and Ogg, don't create a m3u file, archive the CDDB entry files in
       ~/.cddb/\"category\"/ and to eject CD when done, type

              ripit --coder 0,1 --noplaylist --archive --eject --save

       If ripit hangs during CDDB lookup, choose an other mirror instead
       of default freedb.freedb.org server, e. g. at.freedb.org

              ripit --mirror at

       To do the job without any interaction, type

              ripit --nointeraction

       To use a network for encoding, make sure that the paths are equal
       on all machines!

              ripit --sshlist sun,saturn,tethys

       where sun, saturn and tethys  are remote machines on which a user
       can login via ssh without entering a password or passphrase! Note
       that the paths must be equal for the user on all remote machines!
       If the login is different on some machines, try

              ripit --sshlist login1\@sun,login2\@saturn,login3\@tethys

       If there is \"no\" identical path on the remote machines,  then the
       user might enter e.g. /tmp/ as output directory.
       If the file-system is not mounted on each remote machine, one can
       try to copy the wavs to the remote machines using option --scp.

              ripit --sshlist sun,saturn,tethys --scp

FILES
       .ripit/config
              User config file.

       /usr/share/doc/packages/ripit/README
       /usr/share/doc/packages/ripit/HISTORY
       /usr/share/doc/packages/ripit/LICENSE

BUGS
       Probably there are more than some.

SEE ALSO
       cdparanoia(1), lame(1), oggenc(1), flac(1),

AUTHORS
       RipIT is now maintained by Felix Suwald, please send bugs, wishes
       comments to ripit _[at]_ suwald _[dot]_ com. For bugs, wishes and
       comments about lcdproc, contact max.kaesbauer [at] gmail[dot]com.
       Former maintainer:  Mads Martin Joergensen;  RipIT was originally
       developed by Simon Quinn.

DATE
       28 Jun 2005


	\n\n";
}
########################################################################
#
# Change encoding of tags back to iso-8859-1.
#
sub back_encoding {
	my $temp_file = "/tmp/utf8-$$\.txt";
	open(TMP, ">$temp_file") or print "$temp_file $!";
	print TMP $_[0];
	close TMP;
	my $decoded = `/usr/bin/iconv -f UTF-8 -t ISO-8859-1 $temp_file`;
	unlink("/tmp/utf8-$$\.txt");
	return $decoded;
}
########################################################################
#
# Check the preset options.
#
sub check_preset {
	if($preset !~ /^\d/) {
		while($preset !~ /^insane$|^extreme$|^standard$|^medium$/) {
			print "\nPreset should be one of the following words! Please Enter \n";
			print "insane (320\@CBR), extreme (256), standard (192) or medium (160), (standard): ";
			$preset = <STDIN>;
			chomp $preset;
			if($preset eq "") {
				$preset = "standard";
			}
		}
	}
	else {
		while($preset !~ /^32$|^40$|^48$|^56$|^64$|^80$|^96$|^112$|^128$|^160$|^192$|^224$|^256$|^320$/) {
			print "\nPreset should be one of the following numbers! Please Enter \n";
			print "32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256 or 320, (128):\n";
			$preset = <STDIN>;
			chomp $preset;
			if($preset eq "") {
				$preset = 128;
			}
		}
	}
	$preset = "medium" if($preset == 160);
	$preset = "standard" if($preset == 192);
	$preset = "extreme" if($preset == 256);
	$preset = "insane" if($preset == 320);
	$wpreset = $preset;
}
########################################################################
#
# Check sshlist of remote machines and create a hash.
#
sub check_sshlist {
	if(@psshlist) {
		@sshlist = split(/,/, join(',',@psshlist));
	}
	if(@sshlist) {
		$sshflag = 1;
		$wsshlist = join(',',@sshlist);
		# Now create a hash with all machines and a flag.
		# We don't need the flag yet...
		$sshlist{'local'} = '0' if($local == 1);
		foreach (@sshlist) {
			$sshlist{$_} = '0';
		}
	}
	else {
		$sshflag = 0;
	}
}
########################################################################
#
# Dispatcher for encoding on remote machines. If there are no .lock
# files, a ssh command will be passed, else the dispatcher waits until
# an already passed ssh command terminates and removes the lock file.
# The dispatcher checks all machines all 6 seconds until a machine is
# available. If option -scp is used, the dispatcher will not start an
# other job while copying. If it happens, that one machine ended, it
# looks like nothing would happen during scp.
#
sub enc_ssh {
	my $machine;
	my @codwav = ();
	my $delwav = $_[0];
	my $enccom = $_[1];
	my $ripnam = $_[2];
	my $shortname = $_[3];
	my $sufix = $_[4];
	my $old_mp3dir = $mp3dir;
	my $old_ripnam = $ripnam;
	$sshflag = 2;
	while ($sshflag == 2) {
		# Start on the local machine first.
		if(! -r "$mp3dir/local.lock") {
			if($local == 1) {
				$sshflag = 1;
				$machine = "local";
				push @codwav, "$shortname";
			}
		}
		last if($sshflag == 1);
		foreach $_ (keys %sshlist) {
			$machine = $_;
			if(! -r "$mp3dir/$machine.lock") {
				$sshflag = 1;
			}
			# Prepare array @codwav with all tracknames in,
			# which are still in progress.
			else {
				open(LOCK, "$mp3dir/$machine.lock");
				my @locklines = <LOCK>;
				close LOCK;
				my $locklines = @locklines[0] if(@locklines[0]);
				chomp $locklines;
				# Push trackname into array only if not yet
				# present.
				my @presence = grep(/$locklines/, @codwav);
				my $presence = $presence[0];
				push @codwav, "$locklines" if(! $presence);
			}
			last if($sshflag == 1);
		}
		last if($sshflag == 1);
		sleep 3;
	}
	open(LOCKF, ">$mp3dir/$machine.lock");
	print LOCKF "$shortname.$sufix\n";
	close (LOCKF);
	# We need more quotes for the commands (faac,flac,lame,ogg)
	# passed to the remote machen. NOTE: But now pay attention
	# to single quotes in tags. Quote them outside of single quotes!
	# TODO: Please tell me how to quote leading periods, thanks!!!
	if($machine !~ /^local$/) {
		$enccom =~ s/'/'\\''/g;
		$enccom = "'".$enccom."'";
		$enccom = "ssh $machine " . $enccom;
		if($scp) {
			# CREATE the directory:
			# Please tell me why one has to quote the double quotes
			# with a backslash when using ssh!
			$mp3dir = esc_char($mp3dir);
			system("ssh $machine mkdir -p \\\"$mp3dir\\\"");
			# COPY the File:
			# Please tell me why one should NOT quote a file name
			# with blanks when using scp!
			$ripnam = esc_char($ripnam);
			system("scp $ripnam.wav $machine:\"$ripnam.wav\" > /dev/null 2> /dev/null");
		}
	}
	$enccom = $enccom . " > /dev/null 2> /dev/null ";
	if($machine !~ /^local$/ && $scp) {
		$enccom = $enccom . " && scp $machine:\"$ripnam.$sufix\" $ripnam.$sufix > /dev/null 2> /dev/null && ssh $machine rm \"$ripnam.*\" ";
	}
	$enccom = $enccom . " && rm \"$old_mp3dir$machine.lock\" &";
	system("$enccom");
	$mp3dir = $old_mp3dir;
	$ripnam = $old_ripnam;
	# Delete the wav only if all encodings of this track are done!
	# When the last encoding of a track started, its name is pushed
	# into the array @delname. Then the first (oldest) entry is
	# compared to the @codwav array. If still present, nothing
	# happens, else the wav will be deleted and the trackname
	# shifted.
	if($delwav == 1) {
		push @delname, "$shortname";
		my $delflag = 0;
		while ($delflag == 0) {
			my $delname = $delname[0];
			my @delwav = grep(/$delname/, @codwav);
			if($delwav[0] eq "" && $#delname > 1) {
				unlink("$mp3dir/$delname.wav");
				shift(@delname);
				# Prevent endless loop if array is empty.
				$delflag = 1 if($delwav[0] eq "");
			}
			else {
				$delflag = 1;
			}
		}
	}
}
########################################################################
#
# Delete wavs if sshlist was used. TODO: Improve code for following
# situation: if no .lock files are found, but the encoder did not yet
# finish, don't delete the wavs. Do it only after 3*4 seconds timeout
# with no .lock file.
#
sub del_wav {
	my $waitflag = 1;
	sleep 3;
	while ($waitflag <= 3) {
		sleep 3;
		opendir(DIR, "$mp3dir");
		my @locks = readdir(DIR);
		closedir(DIR);
		@locks = grep { /\.lock$/ } @locks;
		$waitflag++ if(! @locks);
		$waitflag = 0 if(@locks);
	}
	if($wav == 0) {
		opendir(DIR, "$mp3dir");
		my @wavs = readdir(DIR);
		closedir(DIR);
		@wavs = grep { /\.wav$/ } @wavs;
		unlink("$mp3dir/$_") foreach (@wavs);
	}
	if($scp) {
		my $old_mp3dir = $mp3dir;
		$mp3dir = esc_char($mp3dir);
		foreach $_ (keys %sshlist) {
			$machine = $_;
			system("ssh $machine rmdir \\\"$mp3dir\\\" > /dev/null 2> /dev/null") if($machine !~ /^local$/);
		}
		$mp3dir = $old_mp3dir;
	}
}
########################################################################
#
# LCDproc subroutines, all credits to Max Kaesbauer. For comments and
# questions contact max [dot] kaesbauer [at] gmail [dot] com.
#

# print

sub plcd {
	($data) = @_;
	print $lcdproc $data."\n";
	$res = <$lcdproc>;
}

# update

sub ulcd {
	if($lcdoline1 ne $lcdline1) {
		$lcdoline1 = $lcdline1;
		plcd("widget_set ripitlcd line1 1 2 {$lcdline1}");
       }
	if($lcdoline2 ne $lcdline2) {
		$lcdoline2 = $lcdline2;
		plcd("widget_set ripitlcd line2 1 3 {$lcdline2}");
	}
	if($lcdoline3 ne $lcdline3) {
		$lcdoline3 = $lcdline3;
		plcd("widget_set ripitlcd line3 1 4 {$lcdline3}");
	}
}

# init

sub init_lcd {
	$lcdproc = IO::Socket::INET->new(
		Proto     => "tcp",
		PeerAddr  => $lcdhost,
		PeerPort  => $lcdport,
	) || die "Cannot connect to LCDproc port\n";
	$lcdproc->autoflush(1);
	sleep 1;

	print $lcdproc "hello\n";
	@lcd_specs = split(/ /,<$lcdproc>);

	$screen{wid} = $lcd_specs[7];
	$screen{hgt} = $lcd_specs[9];
	$screen{cellwid} = $lcd_specs[11];
	$screen{cellhgt} = $lcd_specs[13];

	$screen{pixwid} = $screen{wid}*$screen{cellwid};
	$screen{pixhgt} = $screen{hgt}*$screen{cellhgt};

	fcntl($lcdproc, F_SETFL, O_NONBLOCK);

	plcd("client_set name {ripit.pl}");
	plcd("screen_add ripitlcd");
	plcd("screen_set ripitlcd name {ripitlcd}");

	plcd("widget_add ripitlcd title title");
	plcd("widget_set ripitlcd title {RipIT $version}");

	plcd("widget_add ripitlcd line1 string");
	plcd("widget_add ripitlcd line2 string");
	plcd("widget_add ripitlcd line3 string");
}
########################################################################
#
# Read the CDDB on the local machine.
#
sub read_entry {
	$logfile = $_[0];
	open(LOG, $logfile);
	my @cddblines = <LOG>;
	close(LOG);
	%cd = ();
	# Note that long lines may be split into several lines
	# all starting with the same keyword, e.g. DTITLE.
	if($multi == 0) {
		$cd{raw} = \@cddblines;
		my @artist = grep(s/^DTITLE=//g, @cddblines);
		$artist = join(' / ',@artist);
		$artist =~ s/[\015]//g;
		$artist =~ s/\n\s\/\s//g;
		chomp $artist;
		# Artist is just the first part before first occurrence of
		# of /, album gets all the rest!
		my @disctitle = split(/\s\/\s/, $artist);
		$artist = shift(@disctitle);
		$album = join(' / ',@disctitle);
		chomp $artist;
		chomp $album;
		$cat = $_[1];
		my @genre = grep(s/^DGENRE=//, @cddblines);
		$genre = @genre[0] if($genre eq "");
		$genre =~ s/[\015]//g;
		chomp $genre;
		my @year = grep(s/^DYEAR=//, @cddblines);
		$year = @year[0];
		$year =~ s/[\015]//g;
		chomp $year;
		$trackno = $_[2];
	}
	else {
		my @artist = grep(s/^artist:\s//, @cddblines);
		$artist = @artist[0];
		chomp $artist;
		my @album = grep(s/^title:\s//, @cddblines);
		$album = @album[0];
		chomp $album;
		my @category = grep(s/^category:\s//, @cddblines);
		$cat = @category[0];
		chomp $cat;
		my @genre = grep(s/^genre:\s//, @cddblines);
		$genre = @genre[0];
		chomp $genre;
		my @year = grep(s/^year:\s//, @cddblines);
		$year = @year[0];
		chomp $year;
		my @cddbid = grep(s/^cddbid:\s//, @cddblines);
		$cddbid = @cddbid[0];
		chomp $cddbid;
		my @trackno = grep(s/^trackno:\s//, @cddblines);
		$trackno = @trackno[0];
		chomp $trackno;
	}
	$cd{artist} = $artist;
	$cd{title} = $album;
	$cd{cat} = $cat;
	$cd{genre} = $genre;
	$cd{year} = $year;
	my $i=1;
	my $j=0;
	while($i <= $trackno){
		my @track = ();
		@track = grep(s/^TTITLE$j=//, @cddblines) if($multi == 0);
		@track = grep(s/^track\s$i:\s//, @cddblines) if($multi == 1);
		$track = join(' / ',@track);
		$track =~ s/[\015]//g;
		$track =~ s/\n\s\/\s//g;
		chomp $track;
		$cd{track}[$j]=$track;
		$i++;
		$j++;
	}
}
########################################################################
#
# Delete error.log if there is no Track comment in!
#
sub del_erlog {
	if(-r "$mp3dir/error.log") {
		open(ERR, "$mp3dir/error.log") || print "error.log disappeared!\n";
		my @errlines = <ERR>;
		close ERR;
		my @ulink = grep(/Track /, @errlines);
		if(!@ulink && $multi == 0) {
			unlink("$mp3dir/error.log");
		}
	}
}
########################################################################
#
# Escape special characters when using scp.
#
sub esc_char {
	$_[0] =~ s/\(/\\\(/g;
	$_[0] =~ s/\)/\\\)/g;
	$_[0] =~ s/\[/\\\[/g;
	$_[0] =~ s/\]/\\\]/g;
	$_[0] =~ s/\&/\\\&/g;
	$_[0] =~ s/\'/\\\'/g;
	$_[0] =~ s/ /\\ /g;
	return $_[0];
}
########################################################################
#
# Calculate how much time ripping and encoding needed.
#
sub cal_times{
	$encend = `date \'+%R\'`;
	chomp $encend;
	my @encstart = split(/:/, $encstart);
	my @ripstart = split(/:/, $ripstart);
	my @encend = split(/:/, $encend);
	my @ripend = split(/:/, $ripend);
	$riptime = ($ripend[0]*60+$ripend[1])-($ripstart[0]*60+$ripstart[1]);
	$enctime = ($encend[0]*60+$encend[1])-($encstart[0]*60+$encstart[1]);
}

########################################################################
#
# Thanks to mjb: log info to file.
#
sub log_info {
   if(!defined($infolog)) { return; }
   elsif($infolog eq "") { return; }
   open(SYSLOG, ">>$infolog") or
   print "Can't open info log file <$infolog>.\n";
   print SYSLOG "@_\n";
   close(SYSLOG);
}

########################################################################
#
# Thanks to mjb and Stefan Wartens improvements:
# log_system used throughout in place of system() calls.
#
sub log_system {
   my $P_command = shift;
   if($verbose > 3) {
      # A huge hack only not to interfer with the ripper output.
      if($P_command =~ /faac|flac|lame|machine|mpc|mp4als|oggenc/ &&
         $P_command !~ /cdparanoia|cdda2wav|dagrab|icedax|vorbiscomment/) {
         my $ripmsg = "The audio CD ripper reports: all done!";
         my $ripcomplete = 0;
         if(-r "$wavdir/error.log") {
            open(ERR, "$wavdir/error.log")
               or print "Can not open file error.log!\n";
            my @errlines = <ERR>;
            close(ERR);
            my @ripcomplete = grep(/^$ripmsg/, @errlines);
            $ripcomplete = 1 if(@ripcomplete);
            if(-r "$wavdir/enc.log" && $ripcomplete == 0) {
               open(ENCLOG, ">>$wavdir/enc.log");
               print ENCLOG "\n$P_command\n\n";
               close(ENCLOG);
            }
            else {
               print "system: $P_command\n\n";
            }
         }
      }
      else {
         print "system: $P_command\n\n";
      }
   }

   # Log the system command to logfile unless it's the coverart command
   # for vorbiscomment with the whole binary picture data.
   log_info("system: $P_command") unless($P_command =~ /vorbiscomment/);

   # Start a watch process to check progress of ripped tracks.
   if($parano == 2 && $P_command =~ /^cdparano/
                   && $P_command !~ /-Z/
                   && $P_command !~ /-V/) {
      my $pid = 0;
      # This is probably dangerous, very dangerous because of zombies...
      $SIG{CHLD} = 'IGNORE';
      unless($pid = fork) {
         exec($P_command);
         exit;
      }
      # ... but we check and wait for $pid to finish in subroutine.
      my $result = check_ripper($P_command, $pid);
      waitpid($pid, 0);
      $SIG{CHLD} = 'DEFAULT';
      return $result;
   }
   else {
      system($P_command);
   }

   # system() returns several pieces of information about the launched
   # subprocess squeezed into a 16-bit integer:
   #
   #     7  6  5  4  3  2  1  0  7  6  5  4  3  2  1  0
   #   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   #   |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  [ $? ]
   #   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   #    \_____________________/ \/ \__________________/
   #          exit code        core    signal number
   #
   # To get the exit code, use        ($? >> 8)
   # To get the signal number, use    ($? & 127)
   # To get the dumped core flag, use ($? & 128)

   # Subprocess has been executed successfully.
   return 1 if $? == 0;

   # Subprocess was killed by SIGINT (CTRL-C). Exit RipIT.
   die "\n\nRipit caught a SIGINT.\n" if(( $? & 127) == 2);

   # Subprocess could not be executed or failed.
   return 0;
}
