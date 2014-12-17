#!/bin/sh

. /etc/default/globals

echo

fskcheck check

if [ $? != 1 ]; then
     #fskcheck info "Bitte zuerst PIN Code freischalten ..."
     echo
     echo $(tl "Warning:")
     echo $(tl "Please configure PIN first in main menu")
     exit 1
fi

echo
echo $(tl "Info:")

if [ "$1" = "protect" ]; then

    if [ -f $2/protection.fsk ]; then
        #fskcheck info "Aufzeichnung ist bereits geschützt ..."
        echo $(tl "Recording is already protected")
    else
	touch $2/protection.fsk
	echo $(tl "Recording has been protected")
	#fskcheck info "Die Aufzeichnung wurde geschützt ..."
    fi
fi

if [ "$1" = "unprotect" ]; then

    if [ ! -f $2/protection.fsk ]; then
        #fskcheck info "Aufzeichnung hat keinen Schutz..."
        echo $(tl "Recording has no protection")
    else
	rm -f $2/protection.fsk
	#fskcheck info "Entferne Schutz der Aufzeichnung ..."
	echo $(tl "Childlock has been removed")
    fi
fi

exit 0

