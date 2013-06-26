#include <libnotify/notify.h>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pluginapi.h"
#include "config.h"

using namespace std;

const string &intToStr(int num) {
	size_t len = snprintf(0, 0, "%d", num);
	char buf[len + 1];
	snprintf(buf, len + 1, "%d", num);
	static string result = buf;
	return result;
}

string getBaseFileName(const string &path) {
	size_t i = path.size();
	do {
		--i;
	} while(i && path[i] != '/');
	return path.substr(path[i] != '/'?i:i+1);
}

class notifyProgressHandler : public eventProgress_t{
	private:
		auto_ptr<string> icon;
		void sendNotification(const string &msg) {
#ifdef HAVE_LIBNOTIFY4
			NotifyNotification *notify = notify_notification_new("Instant send", msg.c_str(), (icon.get())?icon->c_str():NULL);
#endif
#ifdef HAVE_LIBNOTIFY1
			NotifyNotification *notify = notify_notification_new("Instant send", msg.c_str(), (icon.get())?icon->c_str():NULL, NULL);
#endif
			GError *err = NULL;
			notify_notification_set_timeout(notify, 3000);
			if(!notify_notification_show(notify, &err)) {
				fprintf(stderr, "Error displaying notification.\n");
				fflush(stderr);
			}
		
		}
	public:
		notifyProgressHandler() : eventProgress_t(), icon(NULL) {
		}

		void setIcon(const string &icon) {
			this->icon.reset(new string(icon));
		}

		void onBegin(fileStatus_t &fStatus) throw() {
			if(fStatus.getDirection() == IS_DIRECTION_DOWNLOAD)
				sendNotification(string("Incoming file ") + getBaseFileName(fStatus.getFileName()) + " (" + intToStr(fStatus.getFileSize()) + "B)");
		}

		void onUpdate(fileStatus_t &fStatus) throw() {
			(void)fStatus;
		}
		void onPause(fileStatus_t &fStatus) throw() {
			(void)fStatus;
		}
		void onResume(fileStatus_t &fStatus) throw() {
			(void)fStatus;
		}
		void onEnd(fileStatus_t &fStatus) throw() {
			string msg = "File transfer ended due to unknown reason.";
			switch(fStatus.getTransferStatus()) {
				case IS_TRANSFER_FINISHED:
					msg = "File " + getBaseFileName(fStatus.getFileName()) + (fStatus.getDirection() == IS_DIRECTION_DOWNLOAD?" received.":" sent.");
					break;
				case IS_TRANSFER_CANCELED_CLIENT:
				case IS_TRANSFER_CANCELED_SERVER:
					 msg = "File " + getBaseFileName(fStatus.getFileName()) + " cancelled";
					 break;
				case IS_TRANSFER_ERROR:
					 msg = "Transfer of file" + getBaseFileName(fStatus.getFileName()) + " failed";
					 break;
			}

			sendNotification(msg);
		}
};

class notifyCreator_t : public eventHandlerCreator_t {
	private:
		notifyProgressHandler progress;
	public:
		notifyCreator_t() : eventHandlerCreator_t(), progress() {
			if(!notify_init("InstantSend")) throw runtime_error("Can't init libnotify");
		}

		void regEvents(eventRegister_t &reg, jsonComponent_t *config) throw() {
			reg.regProgress(progress);
			//(void)config;
			try {
				jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*config);
				progress.setIcon(dynamic_cast<jsonStr_t &>(cfg.gie("icon")).getVal());
			} catch(...) { // ignore any error
				int iconfd = open("/usr/share/instantsend/data/icon_64.png", O_RDONLY); // try default icon path
				if(iconfd > -1) {
					progress.setIcon("/usr/share/instantsend/data/icon_64.png");
					close(iconfd);
				}
			}
		}

		void unregEvents(eventRegister_t &reg) throw() {
			reg.unregProgress(progress);
		}

		const char *getErr() {
			return "No error occured";
		}

		~notifyCreator_t() {
			notify_uninit();
		}
};

extern "C" {
	pluginInstanceCreator_t *getCreator(pluginDestrCallback_t &callback) {
		(void)callback;
		static notifyCreator_t *creator = new notifyCreator_t();
		return creator;
	}
}
