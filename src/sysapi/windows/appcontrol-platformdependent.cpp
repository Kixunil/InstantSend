#include "appcontrol.h"
#include <windows.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <cstdio>

#include "windows-appcontrol.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX    108
#endif

using namespace std;

volatile bool stopApp = false;
volatile bool stopAppFast = false;
unsigned int threadCount = 0;
CRITICAL_SECTION threadCountMutex;

static HANDLE mainThread;

void unfreezeMainThread() {
	ResumeThread(mainThread);
}

void onAppStart(int argc, char **argv) {
	mainThread = GetCurrentThread(); //OpenThread(THREAD_SUSPEND_RESUME, 0, GetCurrentProcessId());
	if(!mainThread) {
		LPVOID lpMsgBuf;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0, NULL );

		fprintf(stderr, "Error opening thread: %s\n", (char *)lpMsgBuf);
		LocalFree(lpMsgBuf);
		exit(1);
	}

	InitializeCriticalSection(&threadCountMutex);

	/*
	for(int i = 1; i < argc; ++i) {
		//fprintf(stderr, "Parsing argument '%s'\n");
		if(string(argv[i]).substr(0, 9) == "--socket=") useSocket(argv[i] + 9); else
		if(string(argv[i]) == "--check") exit(0); else
		if(string(argv[i]) == "--verbose") verbose = 1; else
		if(string(argv[i]) == "--daemon") {
			if(daemon(1, verbose) < 0) throw runtime_error(string("daemon: ") + strerror(errno));
		} else
		if(string(argv[i]).substr(0, 11) == "--pid-file=") writePid(argv[i] + 11);
	}
	*/
}

void onAppStop() {
	DeleteCriticalSection(&threadCountMutex);
}

void freezeMainThread() {
	SuspendThread(mainThread);
}

/*
void threadAboutToExit(pthread_t thread) {
	pthread_mutex_lock(&threadQueueMutex);
	threadQueue.push(thread);
	pthread_mutex_unlock(&threadQueueMutex);
	unfreezeMainThread();
}
*/
