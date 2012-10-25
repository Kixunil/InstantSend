CC=g++
CFLAGS+=-W -Wall -g -O0 -shared -fPIC -I../include

all: ip4tcp.so ip6tcp.so dbus.so desktop-notify.so

ip4tcp.so: ../include/pluginapi.h ip4tcp.cpp
	 $(CC) -o $@ ip4tcp.cpp $(CFLAGS)

ip6tcp.so: ../include/pluginapi.h ip6tcp.cpp
	$(CC) -o $@ ip6tcp.cpp $(CFLAGS)

zenityprogress.so: zenityprogress.cpp
	$(CC) -o $@ zenityprogress.cpp $(CFLAGS)

dbus.so: dbus.cpp
	$(CC) -o $@ dbus.cpp $(CFLAGS) `pkg-config --cflags --libs dbus-1`

desktop-notify.so: desktop-notify.cpp
	$(CC) -o $@ desktop-notify.cpp `pkg-config --cflags --libs libnotify` $(CFLAGS)

clean:
	rm -f *.so
