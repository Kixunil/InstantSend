#!/bin/bash

LOCK_PARENT="$HOME/.instantsend/"
LOCK_NAME="lock"
PID_FILE="$HOME/.instantsend/pid"

is_start() {
	mkdir -p "$LOCK_PARENT" # Make sure parent exists
	if mkdir "$HOME/.instantsend/lock" &> /dev/null; # Obtain lock
	then
		instsendd &> /dev/null & # Run instantsend
		echo $! > "$PID_FILE" # Store PID
		return 0
	else
		echo "Can't obtain lock - InstantSend is probably already running"
		return 1
	fi
}

is_stop() {
	if ISPID=`cat "$PID_FILE" 2>/dev/null`;
	then
		kill "$ISPID" && rm -rf  "$PID_FILE" "$LOCK_PARENT/$LOCK_NAME"
	else
		echo "Can't get PID of InstantSend - probably it's not running"
	fi
}

if [ $# -ne 1 ];
then
	echo "Usage: $0 start|stop|restart|check"
	exit 1
fi

case $1 in
	start)
		is_start && exit 0 || exit 1
		;;
	stop)
		is_stop && exit 0 || exit 1
		;;
	restart)
		is_stop
		is_start && exit 0 || exit 1
		;;
	check)
		if PID=$(cat "$PID_FILE" &>/dev/null) && ps ax | grep -q '^ '"$PID";
		then
			exit 0
		else
			exit 1
		fi
		;;
esac
