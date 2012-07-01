CC=g++
CFLAGS+=-W -Wall -g -shared -fPIC -I../include

all: ip4tcp.so

ip4tcp.so: ip4tcp.cpp
	 $(CC) $(CFLAGS) -o $@ ip4tcp.cpp

clean:
	rm -f *.so
