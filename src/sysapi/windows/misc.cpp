#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <stdexcept>

#include "config.h"

using namespace std;

string combinePath(const string &dir, const string &file) {
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
	char *s = getenv("ProgramFiles");
	if(!s) throw runtime_error("ProgramFiles variable undefined");
	return string(s) + "\\instantsend\\plugins";
#endif
}

string getSystemCfgDir() {
#ifdef SYSCFGNDIR
	return SYSCFGNDIR;
#else
	char *s = getenv("ProgramFiles");
	if(!s) throw runtime_error("ProgramFiles variable undefined");
	return string(s) + "\\instantsend\\config";
#endif
}

void trimSlashes(string &path) {
	size_t i = path.size();
	do {
		--i;
	} while(i && (path[i] == '/' || path[i] == '\\'));
	if(!i) throw runtime_error("Invalid path");
	if(i + 1 < path.size()) path.erase(i + 1);
}

void makePath(const string &path) {
	size_t slashPos = path.size();
	do {
		--slashPos;
	} while(slashPos && path[slashPos] != '/' && path[slashPos] != '\\');
	string directory((path[slashPos] != '/' && path[slashPos] != '\\')?path:path.substr(0, slashPos));
	
	for(size_t i = (directory[0] == '/'?1:0); i < directory.size(); ++i)
	if(directory[i] == '/' || directory[i] == '\\') {
		directory[i] = 0;
		if(mkdir(directory.c_str()) < 0 && errno != EEXIST) throw runtime_error(string("mkdir: ") + strerror(errno));
		directory[i] = '\\';
	}
	if(mkdir(directory.c_str()) < 0 && errno != EEXIST) throw runtime_error(string("mkdir: ") + strerror(errno));
}

bool pathIsUnsafe(const string &path) {
	char state = 1;
	size_t i;
	for(i = 0; i < path.size(); ++i) {
		if(path[i] == '/' || path[i] == '\\') {
			if(state == 3) return true;
			state = 1;
		} else
		if(path[i] == '.' &&state > 0 && state < 3) ++state; else state = 0;
	}

	return i == 3;
}
