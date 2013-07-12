#include <dbus/dbus.h>
#include <string.h>
#include <sys/time.h>
#include <cstdio> // TODO: improve exception handling and error printing

#include <map>
#include <stdexcept>

#include "pluginapi.h"

#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr);

using namespace InstantSend;
using namespace std;

class dbusConnectionHandle {
	public:
		virtual void sendMsg(DBusMessage *dbmsg) = 0;
		virtual void printError() = 0;
};

class progressHandler : public EventProgress {
	private:
		dbusConnectionHandle &dbconnhandle;
		struct timeval lastUpdate;
		//int updatenum;
	public:
		inline progressHandler(dbusConnectionHandle &handle) : EventProgress(), dbconnhandle(handle) {}
		void onBegin(FileStatus &fStatus) throw() {
			DBusMessage *dbmsg = dbus_message_new_signal("/sk/pixelcomp/instantsend", "sk.pixelcomp.instantsend", "onBegin");
			if(!dbmsg) {
				dbconnhandle.printError();
				return;
			}
			uint32_t fileId = fStatus.getId();
			const string &fname = fStatus.getFileName();
			const string &mname = fStatus.getMachineId();
			char *fname_c = new char[fname.size() + 1];
			strcpy(fname_c, fname.c_str());
			char *mname_c = new char[mname.size() + 1];
			strcpy(mname_c, mname.c_str());
			uint64_t fsize = fStatus.getFileSize();
			uint8_t direction = fStatus.getDirection();

			dbus_message_append_args(dbmsg, DBUS_TYPE_UINT32, &fileId, DBUS_TYPE_STRING, &fname_c, DBUS_TYPE_STRING, &mname_c, DBUS_TYPE_UINT64, &fsize, DBUS_TYPE_BYTE, &direction, DBUS_TYPE_INVALID);
			delete[](fname_c);
			delete[](mname_c);
			dbconnhandle.sendMsg(dbmsg);
			dbus_message_unref(dbmsg);
			gettimeofday(&lastUpdate, NULL);
//			updatenum = 0;
		}

		void onUpdate(FileStatus &fStatus) throw() {
			// This prevents overloading dbus
			struct timeval curTime;
			gettimeofday(&curTime, NULL);
			//if((curTime.tv_sec - lastUpdate.tv_sec) * 1000000 + curTime.tv_usec - lastUpdate.tv_usec < 500000) return;
			lastUpdate = curTime;
			//if(++updatenum < 500) return;

			DBusMessage *dbmsg = dbus_message_new_signal("/sk/pixelcomp/instantsend", "sk.pixelcomp.instantsend", "onUpdate");
			if(!dbmsg) {
				dbconnhandle.printError();
				return;
			}
			uint32_t fileId = fStatus.getId();
			uint64_t fbytes = fStatus.getTransferredBytes();
			dbus_message_append_args(dbmsg, DBUS_TYPE_UINT32, &fileId, DBUS_TYPE_UINT64, &fbytes, DBUS_TYPE_INVALID);
			dbconnhandle.sendMsg(dbmsg);
			dbus_message_unref(dbmsg);
		}

		void onPause(FileStatus &fStatus) throw() {
			(void)fStatus;
		}

		void onResume(FileStatus &fStatus) throw() {
			(void)fStatus;
		}

		void onEnd(FileStatus &fStatus) throw() {
			DBusMessage *dbmsg = dbus_message_new_signal("/sk/pixelcomp/instantsend", "sk.pixelcomp.instantsend", "onEnd");
			if(!dbmsg) {
				dbconnhandle.printError();
				return;
			}
			uint32_t fileId = fStatus.getId();
			uint32_t status = fStatus.getTransferStatus();
			dbus_message_append_args(dbmsg, DBUS_TYPE_UINT32, &fileId, DBUS_TYPE_UINT32, &status, DBUS_TYPE_INVALID);
			dbconnhandle.sendMsg(dbmsg);
			dbus_message_unref(dbmsg);
		}
};

class ehCreator_t : public EventHandlerCreator, dbusConnectionHandle {
	private:
		progressHandler progress;
		DBusError dberr;
		DBusConnection *dbconn;
	public:
		ehCreator_t(PluginEnvironment &env) : EventHandlerCreator(env), progress(*this) {
			try {
				dbus_threads_init_default();
				dbus_error_init(&dberr);
				dbconn = dbus_bus_get(DBUS_BUS_SESSION, &dberr);
				if(dbus_error_is_set(&dberr)) {
					string msg = string("Getting session bus failed: ") + dberr.message;
					dbus_error_free(&dberr);
					throw runtime_error(msg);
				}
			}
			catch(exception &e) {
				fprintf(stderr, "DBus plugin exception: %s\n", e.what());
				fflush(stderr);
			}
		}
		const char *getErr() {
			return "No error occured";
		}

		void regEvents(EventRegister &reg, jsonComponent_t *config) throw() {
			(void)config;
			reg.regProgress(progress);
		}

		void unregEvents(EventRegister &reg) throw() {
			reg.unregProgress(progress);
		}

		void sendMsg(DBusMessage *dbmsg) {
			if(!dbus_connection_send(dbconn, dbmsg, NULL)) {
				fprintf(stderr, "Failed to send message");
			}
		}

		void printError() {
				fprintf(stderr, "DBus plugin error: %s\n", dberr.message);
		}

		~ehCreator_t() {
			dbus_connection_unref(dbconn);
			dbus_error_free(&dberr);
			dbus_shutdown();
		}
};

extern "C" {
	PluginInstanceCreator *getCreator(PluginEnvironment &env) {
		static ehCreator_t creator(env);
		return &creator;
	}
}
