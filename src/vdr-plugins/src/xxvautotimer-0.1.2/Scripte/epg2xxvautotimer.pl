#!/usr/bin/perl -w
use warnings;
use strict;
use DBI;
use Socket;

####################################################################################
#
#  This script is used to insert the given timer entry into the XXV AUTOTIMER Database.
#
# based on some XXV stuff  (Ranga)
#
#
## When a command is executed by epgsearch the following parameters are passed to it:
#
# $1: the name of the epg entry
# $2: the start time of the epg entry as time_t value (like in the shutdown script)
# $3: the end time
# $4: the channel of the epg entry
####################################################################################


####################################################################################
# Configuration paramaters
####################################################################################
#  XXV Database 
my $XXVDB="xxv";
my $XXVHOST="localhost";
my $MYSQLPORT=3306;
my $USER="root";
my $PASSWD="root";
####################################################################################
#
my $dsn;
my $dbh;
my $sth;
my @channels;

if( $#ARGV < 3 )
{   
  print("Usage: $0 <epg entry> <start> <end> <channel>\n");
  exit(1);
}


 $dsn="DBI:mysql:database=$XXVDB;host=$XXVHOST;port=$MYSQLPORT";
 $dbh = DBI->connect($dsn, $USER, $PASSWD,
                     { RaiseError => 1, AutoCommit => 0 }) or die $DBI::errstr;

my @autimer = $dbh->selectrow_array('select * from AUTOTIMER;');

my $xxvActive="y";
my $xxvDone="y";
my $xxvSearch="$ARGV[0]";
my $xxvInfields="title";
my $xxvChannels=&PosToChannel($dbh, $ARGV[3]);
my $xxvMinlength="0";
my $xxvPriority="50";
my $xxvLifetime="50";
my $xxvDir="";
my $xxvStart="";
my $xxvStop="";
my $sql = sprintf('insert into AUTOTIMER (Activ,Done,Search,Infields,Channels,Minlength,Priority,Lifetime,Dir,Start,Stop) values ("%s", "%s", "%s", "%s", "%s", "%s", "%s", "%s", "%s", "%s", "%s")',
		    $xxvActive, $xxvDone, $xxvSearch, $xxvInfields, $xxvChannels, $xxvMinlength, $xxvPriority, 
		    $xxvLifetime, $xxvDir, $xxvStart, $xxvStop);

 $dbh->do($sql);
 $dbh->disconnect;


# ------------------
sub PosToChannel {
# ------------------
    my $dbh = shift;
    my $pos = shift || return undef;

    my $sql = sprintf('select Id from CHANNELS where POS = "%d"', $pos);
    my $erg = $dbh->selectall_arrayref($sql);
    return $erg->[0][0];
}


