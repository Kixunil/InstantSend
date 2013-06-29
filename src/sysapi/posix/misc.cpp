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

const char *getFileName(const char *path) {
	const char *fileName = path;
	while(*path) if(*path++ == '/') fileName = path;
	return fileName;
}

void makePath(const string &path) {
	fprintf(stderr, "makePath(\"%s\");\n", path.c_str());
	size_t slashPos(path.rfind('/'));
	fprintf(stderr, "Last slash at %zu\n", slashPos);
	string directory(slashPos == string::npos?path:path.substr(0, slashPos));
	
	for(size_t i = (directory[0] == '/'?1:0); i < directory.size(); ++i)
	if(directory[i] == '/') {
		directory[i] = 0;
		fprintf(stderr, "mkdir(\"%s\");\n", directory.c_str());
		if(mkdir(directory.c_str(), S_IRWXU) < 0 && errno != EEXIST) throw runtime_error(string("mkdir: ") + strerror(errno));
		directory[i] = '/';
	}
	fprintf(stderr, "mkdir(\"%s\");\n", directory.c_str());
	if(mkdir(directory.c_str(), S_IRWXU) < 0 && errno != EEXIST) throw runtime_error(string("mkdir: ") + strerror(errno));
}
