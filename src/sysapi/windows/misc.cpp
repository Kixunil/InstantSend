#include <unistd.h>
#include <string>
#include <stdlib.h>

#include "config.h"

using namespace std;

string combinePath(string dir, string file) {
	return dir + "\\" + file;
}

string getUserDir() {
	char *s = getenv("%APPDATA%");
	if(!s) throw "HOME variable undefined";
	return string(s) + "\\instantsend";
}

string getSystemDataDir() {
	return PREFIX "c:\\Program Files\\instantsend\\data";
}

string getSystemPluginDir() {
#ifdef SYSPLUGINDIR
	return SYSPLUGINDIR;
#else
	return PREFIX "c:\\Program Files\\instantsend\\plugins";
#endif
}

string getSystemCfgDir() {
#ifdef SYSCFGNDIR
	return SYSCFGNDIR;
#else
	return "c:\\Program Files\\instantsend\\config";
#endif
}

const char *getFileName(const char *path) {
	const char *fileName = path;
	while(*path) {
		if(*path == '/' || *path == '\\') fileName = path;
		++path;
	}
	return fileName;
}
