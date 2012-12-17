#!/bin/bash

# Builds debian package for your distribution

INSTANTSEND_VERSION=`grep '^AC_INIT' configure.ac | sed -re 's/^.*\[.*\].*\[([0-9.]*)\].*\[.*\].*$/\1/'`
REVISION=0

# this function was found at http://stackoverflow.com/questions/4023830/bash-how-compare-two-strings-in-version-format
vercomp () {
	if [[ $1 == $2 ]]
	then
		return 0
	fi
	local IFS=.
	local i ver1=($1) ver2=($2)
	# fill empty fields in ver1 with zeros
	for ((i=${#ver1[@]}; i<${#ver2[@]}; i++))
	do
		ver1[i]=0
	done
	for ((i=0; i<${#ver1[@]}; i++))
	do
		if [[ -z ${ver2[i]} ]]
		then
			# fill empty fields in ver2 with zeros
			ver2[i]=0
		fi
		if ((10#${ver1[i]} > 10#${ver2[i]}))
		then
			return 1
		fi
		if ((10#${ver1[i]} < 10#${ver2[i]}))
		then
			return 2
		fi
		done
	return 0
}

if LIBNOTIFY_VERSION=`pkg-config --modversion libnotify`;
then
	echo -n
else
	echo Command \'pkg-config --modversion libnotify\' failed'!' Check whether you have installed pkg-config and libnotify-dev.
	exit 1
fi

vercomp $LIBNOTIFY_VERSION 0.4.5
if [ $? -eq 2 ];
then
	echo 'Unknown Libnotify version!'
	exit 1;
fi

vercomp $LIBNOTIFY_VERSION 0.7.5
if [ $? -eq 2 ];
then
	LIBNOTIFY=1
else
	LIBNOTIFY=4
fi

#There may be more tests/dependencies in the future
if [ $LIBNOTIFY -eq 1 ];
then
	DISTRIBUTION=lucid
fi

if [ $LIBNOTIFY -eq 4 ];
then
	DISTRIBUTION=maya
fi

echo Building InstantSend
echo Version: $INSTANTSEND_VERSION
echo Revision: $REVISION
echo Distribution: $DISTRIBUTION

cp debian/control.$DISTRIBUTION debian/control
cp debian/instantsend-filemanager-nautilus.install.$DISTRIBUTION cp debian/instantsend-filemanager-nautilus.install
sed -e 's/###DISTRIBUTION###/'$DISTRIBUTION'/g' debian/changelog.in > debian/changelog

if [ '!' -f "Makefile.in" ];
then
	automake || exit 1
fi

BINARY=-b

for ARG in "$@";
do
	if [ "$ARG" = "-B" -o "$ARG" = "-b" -o "$ARG" = "-A" ];
	then
		BINARY=""
	fi

done

dpkg-buildpackage $BINARY "$@" || exit 1
