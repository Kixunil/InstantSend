#!/bin/bash

export PATH=$PATH:$HOME/instantsend/src
TARGET="`configtool -C -T | zenity --list --title="Instant Send" --text="Choose target" --column="Targets"`"

while [ $# -gt 0 ];
do
	if client -p -t "$TARGET" -f "$1";
	then
		notify-send -i ~/.instantsend/icon_32.png "Instant Send" "File $1 sent." 
	else
		notify-send -i /usr/share/icons/gnome/32x32/status/dialog-error.png "Instant Send" "Sending of $1 failed!"
	fi | zenity --progress --title="Instant Send" --text="Sending file $1" --auto-kill --auto-close
	shift
done
