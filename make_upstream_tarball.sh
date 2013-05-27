#!/bin/bash

mkdir build_package

for f in `cat upstream_files`;
do
	if [ -f "$f" -o -L "$f" ];
	then
		ln "$f" "build_package/$f" || exit 1
	else
		if [ -d "$f" ];
		then
			mkdir "build_package/$f" || exit 1
		fi
	fi
done

cd build_package

if [ '!' -f "Makefile.in" ];
then
	aclocal
	autoconf
	automake || exit 1
fi

echo Compressing orig tarball
tar -cjvvf ../instantsend_"$1".tar.bz2 ./

cd ..

test "$2" '!=' '-k' && rm -r build_package
