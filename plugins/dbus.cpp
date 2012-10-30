#include <dbus/dbus.h>
#include <string.h>
#include <sys/time.h>

#include <map>
#include <stdexcept>

#include "pluginapi.h"

#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr);

using namespace std;

class dbusConnectionHandle {
	public:
		virtual void sendMsg(DBusMessage *dbmsg) = 0;
		virtual void printError() = 0;
};

class progressHandler : public eventProgress_t {
	private:
		map<fileStatus_t *, FILE *> pipes;
		dbusConnectionHandle &dbconnhandle;
		struct timeval lastUpdate;
		//int updatenum;
	public:
		inline progressHandler(dbusConnectionHandle &handle) : dbconnhandle(handle) {}
		void onBegin(fileStatus_t &fStatus) {
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
			dbconnhandle.sendMsg(dbmsg);
			dbus_message_unref(dbmsg);
			gettimeofday(&lastUpdate, NULL);
//			updatenum = 0;
		}

		void onUpdate(fileStatus_t &fStatus) {
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

		void onPause(fileStatus_t &fStatus) {
			(void)fStatus;
		}

		void onResume(fileStatus_t &fStatus) {
			(void)fStatus;
		}

		void onEnd(fileStatus_t &fStatus) {
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

class ehCreator_t : public eventHandlerCreator_t, dbusConnectionHandle {
	private:
		progressHandler progress;
		DBusError dberr;
		DBusConnection *dbconn;
	public:
		ehCreator_t() : progress(*this) {
			try {
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

		void regEvents(eventRegister_t &reg, jsonComponent_t *config) {
			(void)config;
			reg.regProgress(progress);
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
			dbus_connection_close(dbconn);
			dbus_connection_unref(dbconn);
		}
};

extern "C" {
	pluginInstanceCreator_t *getCreator() {
		static ehCreator_t creator;
		return &creator;
	}
}
