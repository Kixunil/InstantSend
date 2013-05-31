#!/bin/bash

# Builds debian package for your distribution

if [ "$1" = "--keep-build-dir" ];
then
	KEEP_BUILD_DIR=1
	shift
else
	KEEP_BUILD_DIR=0
fi

BINARY=""

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

if LIBCAJA_EXTENSION_VERSION=`pkg-config --modversion libcaja-extension`;
then
	vercomp $LIBCAJA_EXTENSION_VERSION 1.2.1
	if [ $? -eq 2 ];
	then
		echo 'Warning: caja-extension may be too old!'
	fi

	LIBCAJA_EXTENSION=2
else
	LIBCAJA_EXTENSION=0
fi

#There may be more tests/dependencies in the future
if [ $LIBNOTIFY -eq 1 ];
then
	DISTRIBUTION=lucid
fi

if [ $LIBNOTIFY -eq 4 ];
then
	if [ $LIBCAJA_EXTENSION -eq 2 ];
	then
		DISTRIBUTION=maya
	else
		DISTRIBUTION=raspbian
	fi
fi

echo Building InstantSend
echo Version: $INSTANTSEND_VERSION
echo Revision: $REVISION
echo Distribution: $DISTRIBUTION

PACKAGE_NAME="$(sed -e 's/###DISTRIBUTION###/'$DISTRIBUTION'/g' debian/changelog.in | tee debian/changelog | sed -nre '1,1s/([^ ]*) \(([^\)]*)\).*$/\1_\2/p')"

if [ '!' -d build_package ];
then
	./make_upstream_tarball.sh "$INSTANTSEND_VERSION.$REVISION" -k
fi

# Upstream package is same for all distros
ln instantsend_"$INSTANTSEND_VERSION.$REVISION".tar.bz2 "$PACKAGE_NAME.orig.tar.bz2" 2>/dev/null || BINARY="-B" # binary only, if source exists

cd build_package || exit 1 # Prevent serious errors

rm -rf debian
cp -r ../debian ./

cp debian/control.$DISTRIBUTION debian/control
cp debian/instantsend-filemanager-nautilus.install.$DISTRIBUTION debian/instantsend-filemanager-nautilus.install

#remove garbage
rm debian/control.*
rm debian/instantsend-filemanager-nautilus.install.*
rm debian/changelog.in


for ARG in "$@";
do
	if [ "$ARG" = "-B" -o "$ARG" = "-b" -o "$ARG" = "-A" -o "$ARG" = "-F" ];
	then
		BINARY=""
	fi
done

echo "------------------Building package------------------"
dpkg-buildpackage $BINARY "$@" || exit 1

cd ..
if [ $KEEP_BUILD_DIR -eq 0 ];
then
	rm -r build_package
fi
