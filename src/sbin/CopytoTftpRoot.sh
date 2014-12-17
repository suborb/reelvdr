#!/bin/bash

# copy config files to TFPT's root
# so that is it available for all to read.
# copied files are world readable, and only root writeable

TFTP_HOME="/usr/share/reel/configs"

#make sure dir exists
mkdir -p $TFTP_HOME

CopyToTftpRoot()
{
    echo "copying $1"

    case $1 in
    
            "channels.conf")
                if [ -f "/etc/vdr/channels.conf" ]; then
                    cp /etc/vdr/channels.conf     $TFTP_HOME
                    chmod 644 $TFTP_HOME/channels.conf
                else
                    echo "/etc/vdr/channels.conf not found"
                fi
                ;;

           "favourites.conf")
           FAV="/etc/vdr/plugins/reelchannellist/favourites.conf"
                if [ -f $FAV ]; then
                    cp  $FAV $TFTP_HOME
                    chmod 644 $TFTP_HOME/favourites.conf
                else
                    echo "$FAV not found"
                fi
                ;;
                
            "setup.conf")
                if [ -f "/etc/vdr/setup.conf" ]; then
                    cp /etc/vdr/setup.conf        $TFTP_HOME
                    chmod 644 $TFTP_HOME/setup.conf
                else
                    echo "/etc/vdr/setup.conf not found"
                fi
                ;;
            
            "sysconfig")
                if [ -f "/etc/default/sysconfig" ]; then
                    cp /etc/default/sysconfig     $TFTP_HOME
                    chmod 644 $TFTP_HOME/sysconfig
                else
                    echo "/etc/default/sysconfig not found"
                fi
                ;;
            
            "diseqc.conf")
                if [ -f "/etc/vdr/diseqc.conf" ]; then
                    cp /etc/vdr/diseqc.conf       $TFTP_HOME
                    chmod 644 $TFTP_HOME/diseqc.conf
                else
                    echo "/etc/vdr/diseqc.conf not found"
                fi
                ;;
            
                *)
                echo "'$1'?"
                ;;

    esac
}

case $1 in
    "all")
        CopyToTftpRoot "channels.conf"
        CopyToTftpRoot "setup.conf"
        CopyToTftpRoot "diseqc.conf"
        CopyToTftpRoot "sysconfig"
        ;;
    *)
        CopyToTftpRoot $1
        ;;
esac

