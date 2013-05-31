#!/bin/bash

CONF="$HOME/.instantsend/client.cfg"


TARGNUM=0

usage() {
	echo "Usage: $0 [-c CONFIGFILE] [--] FILE [FILE ...]"
}

anim() {	
	ANIM[0]='-'
	ANIM[1]='\\'
	ANIM[2]='|'
	ANIM[3]='/'
	i=0
	while :;
	do 
		echo -ne 'Scanning... '"${ANIM[$i]}"'\r'
		sleep 0.2
		i=$(expr '(' $i + 1 ')' '%' 4)
	done
}

select_target_dialog() {
	LONGEST=`wc -L "$1" | awk '{ print $1; }'`
	TDIR=$(mktemp -d /tmp/isend.XXXXXXX) || exit 1
	if mkfifo "$TDIR/fifo";
	then
		sed 's/:/\n/' "$1" | xargs -d '\n' dialog --menu "Select target" $(expr $2 + 10) $(expr $LONGEST + 10) $2 2>"$TDIR/fifo" & read TARGNUM <"$TDIR/fifo"
	fi
	rm -r $TDIR
	clear
}

select_target_simple() {
	if [ "$2" -gt "$(tput lines)" ];
	then
		less "$1"
	else
		cat "$1"
	fi

	echo 'Choose target (by number):'
	read TARGNUM
}

select_target() {
	if which dialog &>/dev/null;
	then
		select_target_dialog "$@"
	else
		select_target_simple "$@"
	fi
}

if [ $# -lt 1 ];
then
	usage
	exit 1
fi

if [ "$1" = "-h" -o "$1" = "--help" ];
then
	usage
	exit 0
fi

if [ "$1" = "-c" ];
then
	if [ $# -gt 1 ];
	then
		CONF=$2
	else
		echo 'Too few arguments!'
		usage
		exit 1
	fi
	shift
	shift
fi

if [ "$1" = "--" ];
then
	shift
fi

if [ $# -lt 1 ];
then    
	echo 'No file specified!'
	usage
	exit 1
fi 

if which instsend-scan &> /dev/null;
then
	if TMPCONF=$(mktemp /tmp/isend.cfg.XXXXXX);
	then
		anim &
		APID=$!
		instsend-scan -i "$CONF" -o "$TMPCONF"
		kill $APID
		wait $APID 2>/dev/null
		echo -ne '                       \r'
		CONF="$TMPCONF"
	else
		exit 1
	fi
fi

if TMPLIST=$(mktemp /tmp/isend.lst.XXXXXX);
then
	instsend-config -c "$CONF" -T | grep -n '^' > "$TMPLIST"
	TARGETCOUNT=$(wc -l "$TMPLIST" | awk '{ print $1; }')

	if [ "$TARGETCOUNT" -eq 0 ];
	then
		echo 'Error: no targets found!'
		rm -f "$TMPCONF" "$TMPLIST"
		exit 1
	fi

	select_target "$TMPLIST" "$TARGETCOUNT"

	if echo "$TARGNUM" | grep -vq '^[1-9][0-9]*$' || [ "$TARGNUM" -gt "$TARGETCOUNT" -o "$TARGNUM" -lt 1 ];
	then
		echo 'Error: invalid target number'
		rm -f "$TMPCONF" "$TMPLIST"
		exit 1
	fi

	TARGNAME="$(head -$TARGNUM "$TMPLIST" | tail -1 | sed 's/^[0-9]*://')"
	while [ $# -gt 0 ];
	do
		isend -c "$CONF" -t "$TARGNAME" -f "$1"
		shift
	done
else
	rm -f "$TMPCONF"
	exit 1
fi

rm -f "$TMPCONF" "$TMPLIST"
