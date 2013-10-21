#!/bin/sh
echo "Do you want to install Doxygen?"
echo ""
echo "Y/n"
read RESPONSE
if [ $RESPONSE = "y" -o $RESPONSE = "Y" -o $RESPONSE = "" ];then
apt-get install libdbus-1-dev libnotify-dev libgtkmm-2.4-dev libavahi-common-dev libavahi-client-dev avahi-utils libssl-dev zenity doxygen
else
apt-get install libdbus-1-dev libnotify-dev libgtkmm-2.4-dev libavahi-common-dev libavahi-client-dev avahi-utils libssl-dev zenity
fi
