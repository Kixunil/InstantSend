#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <stdexcept>

#include "config.h"

using namespace std;

string combinePath(string dir, string file) {
	return dir + "/" + file;
}

string getUserDir() {
	char *s = getenv("HOME");
	if(!s) throw runtime_error("HOME variable undefined");
	return string(s) + "/.instantsend";
}

string getSystemDataDir() {
	return DATADIR "/instantsend/data";
}

string getSystemPluginDir() {
#ifdef SYSPLUGINDIR
	return SYSPLUGINDIR;
#else
	return LIBDIR "/instantsend/plugins";
#endif
}

string getSystemCfgDir() {
#ifdef SYSCFGNDIR
	return SYSCFGNDIR;
#else
	return "/etc/instantsend";
#endif
}

const char *getFileName(const char *path) {
	const char *fileName = path;
	while(*path) if(*path++ == '/') fileName = path;
	return fileName;
}
