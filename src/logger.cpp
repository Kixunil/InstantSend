#include "logger.h"

using namespace InstantSend;
using namespace std;

void BasicLogger::log(Logger::Level level, const std::string &message) {
	fprintf(mOutFile, "[%s] Application: %s\n", Logger::LevelToStr(level), message.c_str());
}

void BasicLogger::log(Logger::Level level, const std::string &message, const std::string &pluginName) {
	fprintf(mOutFile, "[%s] Plugin %s: %s\n", Logger::LevelToStr(level), pluginName.c_str(), message.c_str());
}

BasicLogger::~BasicLogger() {
	if(mAutoClose) fclose(mOutFile);
}
