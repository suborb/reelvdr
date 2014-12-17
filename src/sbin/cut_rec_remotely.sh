# Script that sends a svdrp command to extrecmenu plugin to cut the given recording
# it reports errors via svdrpsend.sh MESG ...
# and it does NOT report successes

LOGFILE=/tmp/svdrp_log.$$

inLog() 
{ 
     grep ^"$1" $LOGFILE 2>&1 > /dev/null; 
     return $?; 
}

(
svdrpsend.sh $* > $LOGFILE


######### vdr service ready
inLog 220

if [ $? -eq 1 ]; then 
    # service not ready!
    echo "service not ready"
    svdrpsend.sh MESG "Recording not accepted for cutting"
fi


######### vdr server closed
inLog 221

vdr_service_closed=$?
#can svdrp connection remain unclosed ?


######### requested action okay
inLog 250

if [ $? -eq 0 ]; then
    # added recording to avg cutter queue
    echo "success"
    # svdrp message not sent as a message has been shown already
else
    # error in adding 
    echo "error"
    error_str=`grep ^5 $LOGFILE |head -1 |  cut -d' ' -f2-`

    if [ -n $error_str ]; then
        echo $error_str
        svdrpsend.sh MESG $error_str
    fi

fi

) &

