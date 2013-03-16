#include "appcontrol.h"
#include <errno.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <queue>

#include "posix-appcontrol.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX    108
#endif

using namespace std;

static int pipefd[2], sockfd = -1;

volatile bool stopApp = false;
volatile bool stopAppFast = false;
pthread_mutex_t threadQueueMutex, threadCountMutex;
queue<pthread_t> threadQueue;
unsigned int threadCount = 0;

void unfreezeMainThread() {
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

void onAppStart(int argc, char **argv) {
	// Setup pipe
	if(pipe(pipefd) < 0) throw runtime_error(string("pipie: ") + strerror(errno));
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

	for(int i = 1; i < argc; ++i)
		if(string(argv[i]).substr(0, 9) == "--socket=") useSocket(argv[i] + 9);

	pthread_mutex_init(&threadQueueMutex, NULL);
	pthread_mutex_init(&threadCountMutex, NULL);
}

void onAppStop() {
	close(pipefd[0]);
	close(pipefd[1]);
	if(sockfd > -1) close(sockfd);
	pthread_mutex_destroy(&threadQueueMutex);
	pthread_mutex_destroy(&threadCountMutex);
}

void freezeMainThread() {
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
