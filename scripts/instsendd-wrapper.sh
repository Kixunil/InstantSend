#!/bin/bash

LOCK_DIR="$HOME/.instantsend/lock_server"
LOCK_PARENT="$HOME/.instantsend/"
LOCK_NAME="lock"
PID_FILE="$HOME/.instantsend/pid"

is_start() {
	mkdir -p "$LOCK_DIR" # Make sure it exists
	instsend-fifolocker "$LOCK_DIR" instsendd --daemon --pid-file="$PID_FILE" # Run instantsend
	return $?
}

is_stop() {
	instsend-fifolocker "$LOCK_DIR" true
	RET=$?
	if [ $RET -eq 47 ];
	then
		if ISPID=`cat "$PID_FILE" 2>/dev/null`;
		then
			kill -SIGINT "$ISPID"
			return 0
		else
			echo "Can't get PID of InstantSend - probably it's not running"
			return 1
		fi
	else
		echo "InstantSend is not running"
		return 1
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
		instsend-fifolocker "$LOCK_DIR" true
		RET=$?
		if [ $RET -eq 47 ];
		then
			exit 0
		else
			exit 1
		fi
		;;
esac
