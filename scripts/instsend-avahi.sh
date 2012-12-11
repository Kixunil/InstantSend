#!/bin/bash

if [ $# -lt 1 ];
then
	echo Usage: $0 '[-ic] (-c FILENAME|-C)'
	exit 1
fi

avahi-browse -t -r -p -k _instantsend._tcp | grep '^=;[^;]*;IPv4;instantsend;' | awk 'BEGIN {FS=";";}{print "-At"; print "{" "\"" $7 "\" : { \"ways\" : [ { \"plugin\" : \"ip4tcp\", \"config\" : { \"destIP\" : \"" $8 "\", \"destPort\" : " $9 "} } ] } }";}' | xargs -d \\n instsend-config "$1" "$2" "$3"
