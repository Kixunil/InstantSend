#!/bin/bash

if [ $# -lt 1 ];
then
	echo "Usage: $0 PREFIX"
	exit 1
fi

echo '#define PREFIX "'"$1"'"' > config.h
grep -v '^PREFIX=' Makefile > Makefile.tmp
echo "PREFIX=$1" | cat - Makefile.tmp > Makefile
