#include <cstdio>
#include <stdarg.h>

#include "pluginapi.h"

using namespace InstantSend;
using namespace std;

void PluginEnvironment::flog(Logger::Level level, const char *format, ...) {
	va_list vl;
	va_start(vl, format);
	int msgSize = vsnprintf(NULL, 0, format, vl);
	++msgSize; // take '\0' into account
	va_end(vl);
	char buf[msgSize]; // output error will never occur in this case
	va_start(vl, format);
	vsnprintf(buf, msgSize, format, vl);
	va_end(vl);
	log(level, string(buf));
}

void Logger::flog(Level level, const char * format, ...) {
	va_list vl;
	va_start(vl, format);
	int msgSize = vsnprintf(NULL, 0, format, vl);
	++msgSize; // take '\0' into account
	va_end(vl);
	char buf[msgSize]; // output error will never occur in this case
	va_start(vl, format);
	vsnprintf(buf, msgSize, format, vl);
	va_end(vl);
	log(level, string(buf));
}

Logger::~Logger() {}

const char *Logger::LevelToStr(Level level) {
	static const char *names[] = {
		"Fatal error",
		"Security error",
		"Error",
		"Security warning",
		"Warning",
		"Note",
		"Debug",
		"Verbose debug"
	};

	return names[level];
}

PluginInstance::~PluginInstance() {
	mEnv.onInstanceDestroyed();
}

PluginInstanceCreator::~PluginInstanceCreator() {}

#if 0
event_t::~event_t() {
}
#endif
