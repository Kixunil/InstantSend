#include "appcontrol.h"
#include <errno.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <queue>
#include <signal.h>

#include "posix-appcontrol.h"
#include "sysapi.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX    108
#endif
#ifdef __APPLE__
	#define O_CLOEXEC 0
#endif

using namespace InstantSend;
using namespace std;

static int pipefd[2], sockfd = -1;

volatile bool InstantSend::stopApp = false;
volatile bool InstantSend::stopAppFast = false;
pthread_mutex_t threadQueueMutex, threadCountMutex;
queue<pthread_t> threadQueue;
unsigned int threadCount = 0;
int verbose = 0;
int lockfd = -1;

void InstantSend::unfreezeMainThread() {
	char buf = 0;
	write(pipefd[1], &buf, 1);
}

void sighandler(int sig) {
	switch(sig) {
		case SIGCHLD: {
				// Automatic wait
				int status;
				wait(&status);
			}
			break;

		// Stop application
		case SIGTERM:
			stopApp = true;
			stopAppFast = true;
			unfreezeMainThread();
			break;

		case SIGINT:
			stopApp = true;
			unfreezeMainThread();
			break;
	}
}

void useSocket(const char *sockaddr) {
	// Used only to ensure there is single instance of application
	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);

	struct sockaddr_un saddr;
	saddr.sun_family = AF_UNIX;
	strncpy(saddr.sun_path, sockaddr, UNIX_PATH_MAX - 1);
	saddr.sun_path[UNIX_PATH_MAX - 1] = 0;
	if(bind(sockfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_un)) < 0) {
		if(errno == EADDRINUSE) exit(47); // Already running
		throw runtime_error(string("bind: ") + strerror(errno));
	}
}

void writePid(const char *pidFile) {
	lockfd = open(pidFile, O_WRONLY | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP);
	if(lockfd < 0) throw runtime_error(string("open: ") + strerror(errno));
	if(lockf(lockfd, F_TLOCK, 0) < 0) exit(47);
	pid_t pid = getpid();
	char buf[11];
	int len = sprintf(buf, "%u\n", (unsigned int)pid);
	if(write(lockfd, buf, len) < len) throw runtime_error(string("fwrite: ") + strerror(errno));
}

void InstantSend::onAppStart(int argc, char **argv) {
	// Setup pipe
	if(pipe(pipefd) < 0) throw runtime_error(string("pipie: ") + strerror(errno));

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

	// Setup signals
	struct sigaction sa;
	sa.sa_handler = &sighandler;
	sigemptyset(&sa.sa_mask);

	// Ignore SIGCHLD, when child receives SIGSTOP
	sa.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa, NULL);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	pthread_mutex_init(&threadQueueMutex, NULL);
	pthread_mutex_init(&threadCountMutex, NULL);
}

void InstantSend::onAppStop() {
	close(pipefd[0]);
	close(pipefd[1]);
	if(sockfd > -1) close(sockfd);
	if(lockfd > -1) close(sockfd);
	pthread_mutex_destroy(&threadQueueMutex);
	pthread_mutex_destroy(&threadCountMutex);
}

void InstantSend::freezeMainThread() {
	fd_set fds;
	char buf;
	FD_ZERO(&fds);
	FD_SET(pipefd[0], &fds);
	select(pipefd[0] + 1, &fds, NULL, NULL, NULL);
	read(pipefd[0], &buf, 1); // Discard char
	pthread_mutex_lock(&threadQueueMutex);
	while(threadQueue.size()) {
		void *ret;
		pthread_join(threadQueue.front(), &ret);
		threadQueue.pop();
		pthread_mutex_lock(&threadCountMutex);
		--threadCount;
		pthread_mutex_unlock(&threadCountMutex);
	}
	pthread_mutex_unlock(&threadQueueMutex);
}

void threadAboutToExit(pthread_t thread) {
	pthread_mutex_lock(&threadQueueMutex);
	threadQueue.push(thread);
	pthread_mutex_unlock(&threadQueueMutex);
	unfreezeMainThread();
}

Application::Application(int argc, char **argv) : mLogger(stderr), mFilter(mLogger), mSecureStorage(NULL) {
	for(int i = 1; i < argc; ++i) {
		if(string(argv[i]) == "--") break;
		if(string(argv[i]) == "--print-notes") mFilter.setMaxLevel(Logger::Note);
		if(string(argv[i]) == "--debug") mFilter.setMaxLevel(Logger::Debug);
		if(string(argv[i]) == "--verbose-debug") mFilter.setMaxLevel(Logger::VerboseDebug);
		if(string(argv[i]) == "--disable-warnings") mFilter.setMaxLevel(Logger::Error);
	}
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
	getUserDir() + "/files";
}

void Application::fileDir(const std::string &dir) {
	//TODO implement
}

InstantSend::Application *InstantSend::instantSend;
