#!/bin/bash

real_start() {
	echo $$ > /var/lock/instantsend/pid
	instsendd
	rm -rf /var/lock/instantsend
}

is_start() {
	if mkdir /var/lock/instantsend &> /dev/null;
	then
		real_start &
		return 0
	else
		echo "Can't obtain lock - InstantSend is probably already running"
		return 1
	fi
}

is_stop() {
	if ISPID=`cat /var/lock/instantsend/pid 2>/dev/null`;
	then
		killall instsendd && rm -rf  /var/lock/instantsend
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
		if cat /var/lock/instantsend/pid &>/dev/null;
		then
			exit 0
		else
			exit 1
		fi
		;;
esac
