#include <unistd.h>
#include <string>
#include <stdlib.h>

using namespace std;

string combinePath(string dir, string file) {
	return dir + "/" + file;
}

string getStandardDir() {
	char *s = getenv("HOME");
	if(!s) throw "HOME variable undefined";
	return string(s) + "/.instantsend";
}
