#include <libnotify/notify.h>
#include <stdexcept>

#include "pluginapi.h"

using namespace std;

const string &intToStr(int num) {
	size_t len = snprintf(0, 0, "%d", num);
	char buf[len + 1];
	snprintf(buf, len + 1, "%d", num);
	static string result = buf;
	return result;
}

class notifyProgressHandler : public eventProgress_t{
	private:
		string *icon;
		void sendNotification(const string &msg) {
			NotifyNotification *notify = notify_notification_new ("Instant send", msg.c_str(), (icon)?icon->c_str():NULL);
			GError *err = NULL;
			notify_notification_set_timeout(notify, 3000);
			if(!notify_notification_show(notify, &err)) {
				fprintf(stderr, "Error displaying notification.\n");
				fflush(stderr);
			}
		
		}
	public:
		notifyProgressHandler() {
			icon = NULL;
		}

		void setIcon(const string &icon) {
			this->icon = new string(icon);
		}

		void onBegin(fileStatus_t &fStatus) {
			sendNotification("Incoming file " + fStatus.getFileName() + " (" + intToStr(fStatus.getFileSize()) + ")");
		}

		void onUpdate(fileStatus_t &fStatus) {
			(void)fStatus;
		}
		void onPause(fileStatus_t &fStatus) {
			(void)fStatus;
		}
		void onResume(fileStatus_t &fStatus) {
			(void)fStatus;
		}
		void onEnd(fileStatus_t &fStatus) {
			string msg = "File transfer ended due to unknown reason.";
			switch(fStatus.getTransferStatus()) {
				case IS_TRANSFER_FINISHED:
					msg = "File " + fStatus.getFileName() + " finished.";
					break;
				case IS_TRANSFER_CANCELED_CLIENT:
				case IS_TRANSFER_CANCELED_SERVER:
					 msg = "File " + fStatus.getFileName() + "cancelled";
					 break;
				case IS_TRANSFER_ERROR:
					 msg = "Transfer of file" + fStatus.getFileName() + " failed";
					 break;
			}

			sendNotification(msg);
		}
};

class notifyCreator_t : public eventHandlerCreator_t {
	private:
		notifyProgressHandler progress;
	public:
		notifyCreator_t() {
			if(!notify_init("InstantSend")) throw runtime_error("Can't init libnotify");
		}

		void regEvents(eventRegister_t &reg, jsonComponent_t *config) {
			reg.regProgress(progress);
			//(void)config;
			try {
				jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*config);
				progress.setIcon(dynamic_cast<jsonStr_t &>(cfg.gie("icon")).getVal());
			} catch(...) { // ignore any error
			}
		}

		const char *getErr() {
			return "No error occured";
		}

		~notifyCreator_t() {
			notify_uninit();
		}
};

extern "C" {
	pluginInstanceCreator_t *getCreator() {
		static notifyCreator_t creator;
		return &creator;
	}
}
