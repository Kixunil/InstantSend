#include "appcontrol.h"
#include <windows.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <cstdio>

#include "windows-appcontrol.h"
#include "sysapi.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX    108
#endif

using namespace InstantSend;
using namespace std;

volatile bool InstantSend::stopApp = false;
volatile bool InstantSend::stopAppFast = false;
unsigned int threadCount = 0;
CRITICAL_SECTION threadCountMutex;

static HANDLE mainThread;

void InstantSend::unfreezeMainThread() {
	ResumeThread(mainThread);
}

void InstantSend::onAppStart(int argc, char **argv) {
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

void InstantSend::onAppStop() {
	DeleteCriticalSection(&threadCountMutex);
}

void InstantSend::freezeMainThread() {
	SuspendThread(mainThread);
}

Application::Application(int argc, char **argv) : mLogger(stderr) {
}

void InstantSend::Application::requestStop() {
	stopApp = true;
	unfreezeMainThread();
}

void InstantSend::Application::requestFastStop() {
	stopApp = true;
	stopAppFast = true;
	unfreezeMainThread();
}

const std::string &Application::systemDataDir() {
	return getSystemDataDir();
}

const std::string &Application::systemConfigDir() {
	return getSystemCfgDir();
}

const std::string &Application::userDataDir() {
	return getUserDir();
}

const std::string &Application::fileDir() {
	getUserDir() + "\\files";
}

void Application::fileDir(const std::string &dir) {
	//TODO implement
}

InstantSend::Application *InstantSend::instantSend;
