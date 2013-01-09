#include "appcontrol.h"
#include <errno.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

static int pipefd[2];

volatile bool stopApp = false;
volatile bool stopAppFast = false;

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
}

void onAppStop() {
	close(pipefd[0]);
	close(pipefd[1]);
}

void freezeMainThread() {
	fd_set fds;
	char buf;
	FD_ZERO(&fds);
	FD_SET(pipefd[0], &fds);
	select(pipefd[0] + 1, &fds, NULL, NULL, NULL);
	read(pipefd[0], &buf, 1); // Discard char
}


