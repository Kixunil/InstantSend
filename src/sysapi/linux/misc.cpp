#include <unistd.h>
#include <string>
#include <stdlib.h>

#include "config.h"

using namespace std;

string combinePath(string dir, string file) {
	return dir + "/" + file;
}

string getUserDir() {
	char *s = getenv("HOME");
	if(!s) throw "HOME variable undefined";
	return string(s) + "/.instantsend";
}

string getSystemDataDir() {
	return PREFIX "/share/instantsend";
}

string getSystemPluginDir() {
	return PREFIX "/lib/instantsend/plugins";
}

string getSystemCfgDir() {
	return "/etc/instantsend";
}

const char *getFileName(const char *path) {
	const char *fileName = path;
	while(*path) if(*path++ == '/') fileName = path;
	return path;
}
