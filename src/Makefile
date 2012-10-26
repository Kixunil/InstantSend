CFLAGS+=-W -Wall -g -O0 -I../include -I. -I../
LDFLAGS=-ldl -pthread -rdynamic 

all: client server configtool widget-gtk

client: json.o pluginloader.o pluginloader-platformdependent.o pluginlist.o plugin.o sysapi.o eventsink.o ../include/pluginapi.h client.cpp
	$(CXX) $(CFLAGS) -o $@ json.o pluginloader-platformdependent.o pluginloader.o pluginlist.o plugin.o sysapi.o eventsink.o client.cpp $(LDFLAGS)

server: json.o pluginloader.o pluginloader-platformdependent.o pluginlist.o plugin.o sysapi.o multithread.o filewriter.o filelist.o eventsink.o ../include/pluginapi.h server.cpp
	$(CXX) $(CFLAGS) -o $@ json.o pluginloader-platformdependent.o pluginloader.o pluginlist.o plugin.o sysapi.o multithread.o filewriter.o filelist.o eventsink.o server.cpp $(LDFLAGS)

widget-gtk: json.o sysapi.o widget-gtk.cpp
	$(CXX) $(CFLAGS) -o $@ widget-gtk.cpp json.o sysapi.o `pkg-config --cflags --libs gtkmm-2.4` `pkg-config --cflags --libs dbus-1`

configtool: configtool.cpp json.o
	$(CXX) $(CFLAGS) -o $@ json.o configtool.cpp

libinstantsend.o: libinstantsend.cpp
	$(CXX) $(CFLAGS) -c -o $@ libinstantsend.cpp

json.o: json.cpp
	$(CXX) $(CFLAGS) -c -o $@ json.cpp

pluginlist.o: pluginlist.cpp
	$(CXX) $(CFLAGS) -c -o $@ pluginlist.cpp

plugin.o: plugin.cpp
	$(CXX) $(CFLAGS) -c -o $@ plugin.cpp

environment.o: sysapi/linux/environment.cpp
	$(CXX) $(CFLAGS) -c -o $@ sysapi/linux/environment.cpp

pluginloader.o: pluginloader.cpp
	$(CXX) $(CFLAGS) -c -o $@ pluginloader.cpp

pluginloader-platformdependent.o: sysapi/linux/pluginloader-linux.cpp
	$(CXX) $(CFLAGS) -c -o $@ sysapi/linux/pluginloader-linux.cpp

sysapi.o: sysapi/linux/misc.cpp
	$(CXX) $(CFLAGS) -c -o $@ sysapi/linux/misc.cpp

multithread.o: sysapi/linux/multithread.cpp
	$(CXX) $(CFLAGS) -pthread -c -o $@ sysapi/linux/multithread.cpp

filewriter.o: filewriter.cpp
	$(CXX) $(CFLAGS) -c -o $@ filewriter.cpp

filelist.o: filelist.cpp
	$(CXX) $(CFLAGS) -c -o $@ filelist.cpp

eventsink.o: eventsink.cpp
	$(CXX) $(CFLAGS) -c -o $@ eventsink.cpp

clean:
	rm -f *.o client server configtool widget-gtk
