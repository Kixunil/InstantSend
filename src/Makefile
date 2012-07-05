CC=g++
CFLAGS+=-W -Wall -g -I../include -I.
LDFLAGS=-ldl -pthread -rdynamic 

all: client server configtool

client: json.o pluginloader.o pluginloader-platformindependent.o pluginlist.o plugin.o client.cpp
	$(CC) $(CFLAGS) -o $@ json.o pluginloader.o pluginloader-platformindependent.o pluginlist.o plugin.o client.cpp $(LDFLAGS)

server: json.o pluginloader.o pluginloader-platformindependent.o pluginlist.o plugin.o server.cpp
	$(CC) $(CFLAGS) -o $@ json.o pluginloader.o pluginloader-platformindependent.o pluginlist.o plugin.o server.cpp $(LDFLAGS)

configtool: configtool.cpp json.o
	$(CC) $(CFLAGS) -o $@ json.o configtool.cpp

libinstantsend.o: libinstantsend.cpp
	$(CC) $(CFLAGS) -c -o $@ libinstantsend.cpp

json.o: json.cpp
	$(CC) $(CFLAGS) -c -o $@ json.cpp

pluginlist.o: pluginlist.cpp
	$(CC) $(CFLAGS) -c -o $@ pluginlist.cpp

plugin.o: plugin.cpp
	$(CC) $(CFLAGS) -c -o $@ plugin.cpp

environment.o: sysapi/linux/environment.cpp
	$(CC) $(CFLAGS) -c -o $@ sysapi/linux/environment.cpp

pluginloader-platformindependent.o: pluginloader.cpp
	$(CC) $(CFLAGS) -c -o $@ pluginloader.cpp

pluginloader.o: sysapi/linux/pluginloader.cpp
	$(CC) $(CFLAGS) -c -o $@ sysapi/linux/pluginloader.cpp

clean:
	rm -f *.o client server configtool
