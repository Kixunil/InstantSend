#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "config.h"

using namespace std;

string combinePath(const string &dir, const string &file) {
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

size_t getFileName(const string &path) {
	size_t i = path.size();
	do {
		--i;
	} while(i && path[i] != '/');
	if(path[i] == '/') ++i;
	return i;
}

void trimSlashes(string &path) {
	size_t i = path.size();
	do {
		--i;
	} while(i && path[i] == '/');
	if(!i) path = "/"; else if(i + 1 < path.size()) path.erase(i + 1);
}

void makePath(const string &path) {
	size_t slashPos(path.rfind('/'));
	string directory(slashPos == string::npos?path:path.substr(0, slashPos));
	
	for(size_t i = (directory[0] == '/'?1:0); i < directory.size(); ++i)
	if(directory[i] == '/') {
		directory[i] = 0;
		if(mkdir(directory.c_str(), S_IRWXU) < 0 && errno != EEXIST) throw runtime_error(string("mkdir: ") + strerror(errno));
		directory[i] = '/';
	}
	if(mkdir(directory.c_str(), S_IRWXU) < 0 && errno != EEXIST) throw runtime_error(string("mkdir: ") + strerror(errno));
}

bool pathIsUnsafe(const string &path) {
	char state = 1;
	size_t i;
	for(i = 0; i < path.size(); ++i) {
		if(path[i] == '/') {
			if(state == 3) return true;
			state = 1;
		} else
		if(path[i] == '.' &&state > 0 && state < 3) ++state; else state = 0;
	}

	return i == 3;
}
