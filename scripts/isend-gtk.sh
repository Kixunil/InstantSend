#!/bin/bash

if [ $# -lt 1 ];
then
	echo "Usage: $0 [-c CLIENT_CONFIG_FILE] file [file ...]"
	exit 1
fi

if [ "$1" = "-c" ];
then
	CFGFLAG=-c
	CFG="$2"
	ISENDFLAG=-c
	shift
	shift
else
	CFGFLAG=-C
	CFG=""
	ISENDFLAG=""
fi

if TARGET="`instsend-config $CFGFLAG "$CFG" -T | zenity --list --title="Instant Send" --text="Choose target" --column="Targets"`";
then
	while [ $# -gt 0 ];
	do
		if isend "$ISENDFLAG" "$CFG" -p -t "$TARGET" -f "$1";
		then
			notify-send -i ~/.instantsend/icon_32.png "Instant Send" "File $1 sent." 
		else
			notify-send -i /usr/share/icons/gnome/32x32/status/dialog-error.png "Instant Send" "Sending of $1 failed!"
		fi | zenity --progress --title="Instant Send" --text="Sending file $1" --auto-kill --auto-close
		shift
	done
fi
