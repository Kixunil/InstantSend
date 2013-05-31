#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

#define TRYFIFO(POS) do { if(tryFifo(names[POS])) { perror("mkfifo"); return 2; } } while(0)
#define MKNAME(POS, NAME) do { if(mkName(names + POS, argv[1], NAME)) return 2; } while(0)
#define L(LOCK) do { if((locks[LOCK] = lockF(names[LOCK + 1])) < 0) return 2; } while(0)
#define U(LOCK) unlockF(locks[LOCK])
#define W(LOCK) do { if((waitF(names[LOCK + 1])) < 0) return 2; } while(0)
#define T(LOCK) testF(names[LOCK + 1])

void printHelp(const char *progName) {
	fprintf(stderr, "Usage: %s DIRECTORY PROGRAM [ARGS ...]\n"
			"This program creates reliable process singleton with these properties:\n"
			"At most one program per directory\n"
			"Lock won't be in unusable state if any process fail at any time\n"
			"After PROGRAM closes it's lock file descriptor, exits or fails, another program can be run\n"
			"Attemp to run more programs at the same time is safe\n"
			"Locks are not protected against users actions - use apropriate file permissions!\n"
			"Usage of EMPTY DIRECTORY is strongly encouraged\n", progName);
}

int mkName(char **outbuf, const char *dir, const char *name) {
	size_t dirLen = strlen(dir), nameLen = strlen(name);
	*outbuf = malloc(dirLen + nameLen + 2);
	if(!*outbuf) return -1;
	strcpy(*outbuf, dir);
	(*outbuf)[dirLen] = '/';
	strcpy((*outbuf) + dirLen + 1, name);
	return 0;
}

int tryFifo(const char *path) {
	errno = 0;
	return mkfifo(path, S_IRUSR | S_IWUSR) < 0 && errno != EEXIST;
}

inline int lockF(const char *path) {
	errno = 0;
	return open(path, O_RDWR, 0);
}

inline void unlockF(int fd) {
	errno = 0;
	close(fd);
}

int waitF(const char *path) {
	errno = 0;
	int tmpfd = open(path, O_RDWR); // makes sure open won't block
	if(tmpfd < 0) return - 1;
	int waitfd = open(path, O_RDONLY);
	if(waitfd < 0) return -1;
	close(tmpfd);
	char c;
	read(tmpfd, &c, 1); // blocks if opened
	close(waitfd);
	return 0;
}

int testF(const char *path) {
	errno = 0;
	int tmpfd = open(path, O_RDONLY | O_NONBLOCK);
	if(tmpfd < 0) {
		if(errno == EWOULDBLOCK) return 0; else return -1;
	} else {
		char c;
		ssize_t ret = read(tmpfd, &c, 1);
		if(ret == 0) {
			close(tmpfd);
			return 0;
		} else {
			if(errno == EWOULDBLOCK) return 1; else return -1;
		}
	}
}

int lockEx(const char *path) {
	int tmpfd = open(path, O_WRONLY | O_CREAT | O_EXCL);
	if(tmpfd < 0) return 0;
	close(tmpfd);
	return 1;
}

inline void unlockEx(const char *path) {
	unlink(path);
}

int main(int argc, char **argv) {
	if(argc < 3) {
		printHelp(*argv);
		return 1;
	}

	if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printHelp(*argv);
		return 0;
	}

	char * names[6];

	MKNAME(0, "exclusive_lock");
	MKNAME(1, "fifo1");
	MKNAME(2, "fifo2");
	MKNAME(3, "fifo3");
	MKNAME(4, "fifo4");
	MKNAME(5, "fifo5");

	TRYFIFO(1);
	TRYFIFO(2);
	TRYFIFO(3);
	TRYFIFO(4);
	TRYFIFO(5);

	int locks[5];
	do {
		W(2);
		L(1);
		L(0);
		W(3);
		if(lockEx(names[0])) {
			// We have exclusive lock
			L(2);

			// This "final" trick will reduce need of opened file descriptors to one
			int exe = 0;
			int ret = T(4);
			if(ret < 0) {
				perror("open");
				return 2;
			}
			if(!ret) { // this is safe, because at most one program will do this
				L(4);
				exe = 1;
			}

			// End of locked section
			unlockEx(names[0]); // we dont have to do this - if program crashes, it's automatically recovered
			U(1);
			U(0);
			U(2);

			if(exe) {
				// Execute program here
				execvp(argv[2], argv + 2);
				perror("execvp");
				return 3;
			}
			return 47; // already running
		} else {
			// Already locked - let's find out, if it's valid lock
			L(2);
			U(0);
			W(0);
			U(1);
			L(3);
			if(!T(1)) unlockEx(names[0]);
			U(3);
			U(2);
		}
	} while(1);
}
