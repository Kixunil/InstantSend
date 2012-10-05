#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include <map>

#include "pluginapi.h"

#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr);

using namespace std;

class progressDisplay : public eventProgress_t {
	private:
		map<fileStatus_t *, FILE *> pipes;
	public:
		void onBegin(fileStatus_t &fStatus) {
/*			printf("File: %s begins transferring\n", fStatus.getFileName().c_str());
			fflush(stdout);*/

			int pfd[2];
			if(pipe(pfd)) {
				D("pipe() failed");
				return;
			}
			FILE *writeend = fdopen(pfd[1], "w");
			if(!writeend) {
				close(pfd[0]);
				close(pfd[1]);
				D("fdopen() failed");
				return;
			}
			pipes[&fStatus] = writeend;
			pid_t pid = fork();
			if(pid < 0) {
				close(pfd[0]);
				fclose(writeend);
				pipes.erase(&fStatus);
				D("fork() failed");
				return;
			}
			if(!pid) {
				if(dup2(pfd[0], 0) < 0) {
					D("dup2() failed");
					_exit(1);
				}
				fclose(writeend);
				char *bin = "/usr/bin/zenity";
				char *progress = "--progress";
				char *title = "--title=InstantSend";
				char *text = "--text=Transferring file";
				char *autoClose = "--auto-close";
				char * const argv[6] = {bin, progress, title, text, autoClose, NULL};
				execv(bin, argv);
				D("execv() failed");
				_exit(1);
			}
			close(pfd[0]);
		}

		void onUpdate(fileStatus_t &fStatus) {
/*			printf("File: %s; Transfered %zu bytes out of %zu (%zu%%)\r", fStatus.getFileName().c_str(), fStatus.getTransferredBytes(), fStatus.getFileSize(), 100 * fStatus.getTransferredBytes() / fStatus.getFileSize());
			fflush(stdout);*/
			map<fileStatus_t *, FILE *>::iterator it = pipes.find(&fStatus);
			if(it == pipes.end()) {
				D("fileStatus not found");
				return;
			}
			if(fprintf(it->second, "%zu\n", 100 * fStatus.getTransferredBytes() / fStatus.getFileSize()) < 0) { D("write to pipe failed"); }
			fflush(it->second);
		}
		void onPause(fileStatus_t &fStatus) {
			printf("File: %s paused\n", fStatus.getFileName().c_str());
			fflush(stdout);
		}
		void onResume(fileStatus_t &fStatus) {
			printf("File: %s resumed\n", fStatus.getFileName().c_str());
			fflush(stdout);
		}
		void onEnd(fileStatus_t &fStatus) {
			switch(fStatus.getTransferStatus()) {
				case IS_TRANSFER_FINISHED:
					printf("File: %s finished\n", fStatus.getFileName().c_str());
					fflush(stdout);
					break;
				case IS_TRANSFER_CANCELED_CLIENT:
					printf("File: %s cancelled by client\n", fStatus.getFileName().c_str());
					fflush(stdout);
					break;
				case IS_TRANSFER_CANCELED_SERVER:
					printf("File: %s cancelled by server\n", fStatus.getFileName().c_str());
					fflush(stdout);
					break;
				case IS_TRANSFER_ERROR:
					printf("File: %s cancelled because of error\n", fStatus.getFileName().c_str());
					fflush(stdout);
					break;
			}

			map<fileStatus_t *, FILE *>::iterator it = pipes.find(&fStatus);
			if(it == pipes.end()) {
				return;
			}

			fclose(it->second);
			pipes.erase(it);

			pid_t pid = fork();
			if(pid < 0) return;

			if(pid) {
				int ret = 0;
				waitpid(pid, &ret, 0);
				if(!ret) {
					pid_t xop = fork();
					if(xop) return;
					char *bin = "/usr/bin/xdg-open";
					char *arg = new char[fStatus.getFileName().size() + 1];
					strcpy(arg, fStatus.getFileName().c_str());
					char *argv[3] = {bin, arg, NULL};

					execv(bin, argv);
					_exit(1);
				}
			}
			char *bin = "/usr/bin/zenity";
			char *dtype = "--question";
			char *title = "--title=InstantSend";
			string question = "--text=File " + fStatus.getFileName() + " received. Would you like to open it?";
			char *text = new char[question.size() + 1];
			strcpy(text, question.c_str());
			char *argv[5] = { bin, dtype, title, text, NULL};

			execv(bin, argv);
			_exit(0);
		}
};

class ehCreator_t : public eventHandlerCreator_t {
	private:
		progressDisplay progress;
	public:
		const char *getErr() {
			return "No error occured";
		}

		void regEvents(eventRegister_t &reg, jsonComponent_t *config) {
			reg.regProgress(progress);
		}
};

extern "C" {
	pluginInstanceCreator_t *getCreator() {
		static ehCreator_t creator;
		return &creator;
	}
}
