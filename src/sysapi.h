#include <string>

using namespace std;

string combinePath(const string &dir, const string &file);
string getUserDir();
string getSystemDataDir();
string getSystemPluginDir();
string getSystemCfgDir();

size_t getFileName(const string &path);
void trimSlashes(string &path);

void makePath(const string &path);
bool pathIsUnsafe(const string &path);
