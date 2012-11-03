#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <stdexcept>

#include "config.h"

using namespace std;

string combinePath(string dir, string file) {
	return dir + "\\" + file;
}

string getUserDir() {
	char *s = getenv("AppData");
	if(!s) throw runtime_error("APPDATA variable undefined");
	return string(s) + "\\instantsend";
}

string getSystemDataDir() {
	char *s = getenv("ProgramFiles");
	if(!s) throw runtime_error("ProgramFiles variable undefined");
	return string(s) + "\\instantsend\\data";
}

string getSystemPluginDir() {
#ifdef SYSPLUGINDIR
	return SYSPLUGINDIR;
#else
	return getSystemDataDir() + "\\instantsend\\plugins";
#endif
}

string getSystemCfgDir() {
#ifdef SYSCFGNDIR
	return SYSCFGNDIR;
#else
	return getSystemDataDir() + "\\instantsend\\config";
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
