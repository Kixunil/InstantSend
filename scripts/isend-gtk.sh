#!/bin/bash

TMPCFG="$(mktemp /tmp/instsend-targets.XXXXXX)"

scantargets() {
	echo Scanning...
	if [ $CFGFLAG = "-c" ];
	then
		instsend-scan -i "$CFG" -o "$TMPCFG"
	else
		instsend-scan -o "$TMPCFG"
	fi
	CFG="$TMPCFG"
	CFGFLAG=-c
	ISENDFLAG=-c
	echo 100
}

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

if which instsend-scan &>/dev/null;
then
	scantargets | zenity --progress --title="Instant Send" --text="Searching for targets" --pulsate --auto-kill --auto-close
	CFG="$TMPCFG"
	CFGFLAG=-c
	ISENDFLAG=-c
fi

if TARGET="`instsend-config $CFGFLAG "$CFG" -T | zenity --list --title="Instant Send" --text="Choose target" --column="Targets"`";
then
	while [ $# -gt 0 ];
	do
		CURFILE="$(echo $1 | sed 's/^file:\/\///')"
		if [ -d "$CURFILE" ];
		then
			continue # Skip directories
		fi
		if isend "$ISENDFLAG" "$CFG" -p -t "$TARGET" -f "$CURFILE";
		then
			notify-send -i ~/.instantsend/icon_32.png "Instant Send" "File $CURFILE sent." 
		else
			notify-send -i /usr/share/icons/gnome/32x32/status/dialog-error.png "Instant Send" "Sending of $CURFILE failed!"
		fi | zenity --progress --title="Instant Send" --text="Sending file $CURFILE" --auto-kill --auto-close
		shift
	done
fi

if [ -f "$TMPCFG" ];
then
	rm -f "$TMPCFG"
fi
