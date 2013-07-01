#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <stdexcept>

#ifndef WINDOWS
#include <sys/types.h>
#include <unistd.h>
#endif

#include "pluginapi.h"
#include "pluginlist.h"
#include "sysapi.h"
#include "eventsink.h"
#include <stdexcept>

int outputpercentage = 0;

inline size_t getBaseFileName(const string &path) {
	return getFileName(path);
}

class StatusReporter : public thread_t {
	public:
		StatusReporter(fileStatus_t &fs) : mFileStatus(fs), mRunning(true), mSem(0) {}
		void run() {
			while(mRunning) {
				usleep(500000);
				bcastProgressUpdate(mFileStatus);
				if(outputpercentage) {
					printf("%ld\n", 100*mFileStatus.getTransferredBytes() / mFileStatus.getFileSize());
					fflush(stdout);
				}
			}
			++mSem;
		}

		void join() {
			mRunning = false;
			--mSem;
		}

		bool autoDelete() {
			return true;
		}
	private:
		fileStatus_t &mFileStatus;
		volatile bool mRunning;
		Semaphore mSem;

};

class FileReader : public fileStatus_t {
	public:
		FileReader(const string &name, const string &target) : mName(name), mTarget(target), mFile(name), mSize(mFile.size()), mBytes(0),
#ifndef WINDOWS
		mId(getpid()),
#endif
		mStatus(IS_TRANSFER_CONNECTING),
		mReporter(*new StatusReporter(*this))
	       	{
			bcastProgressBegin(*this);
			mReporter.start();
		}

		string getFileName() {
			return mName;
		}

		File::Size getFileSize() {
			return mSize;
		}

		string getMachineId() {
			return mTarget;
		}

		File::Size getTransferredBytes() {
			return mBytes;
		}

		int getTransferStatus() {
			return mStatus;
		}

		int getId() {
			return mId;
		}

		char getDirection() {
			return IS_DIRECTION_UPLOAD;
		}

		void pauseTransfer() {}
		void resumeTransfer() {}

		int send(peer_t &client, const string &basename) {
			try {
				fprintf(stderr, "File size: %llu\n", (unsigned long long)mSize);
				fflush(stderr);

				mStatus = IS_TRANSFER_IN_PROGRESS;

				bcastProgressUpdate(*this);

				auto_ptr<anyData> data(allocData(DMAXSIZE+1));

				jsonObj_t msgobj;
				msgobj.insertNew("service", new jsonStr_t("filetransfer"));
				msgobj.insertNew("filename", new jsonStr_t(basename.c_str()));
				msgobj.insertNew("filesize", new jsonInt_t((intL_t)mSize));
				string msg = msgobj.toString();
				strcpy(data->data, msg.c_str());
				data->size = msg.size() + 1;
				//msgobj.deleteContent();

				if(!client.sendData(data.get())) throw runtime_error("Can't send!");
				data->size = DMAXSIZE;
				if(!client.recvData(data.get())) throw runtime_error("No response");
				data->data[data->size] = 0;

				msg = string(data->data);
				msgobj = jsonObj_t(&msg);
				if(dynamic_cast<jsonStr_t &>(msgobj.gie("service")) != "filetransfer") throw runtime_error("Unknown protocol");
				jsonStr_t &action = dynamic_cast<jsonStr_t &>(msgobj.gie("action"));
				if(action.getVal() != string("accept")) {
					try {
						jsonStr_t &reason = dynamic_cast<jsonStr_t &>(msgobj.gie("reason"));
						mStatus = IS_TRANSFER_ERROR;
						throw runtime_error(string("File rejected: ") + reason.getVal());
					} catch(jsonNotExist) {
						mStatus = IS_TRANSFER_ERROR;
						throw runtime_error("File rejected: unknown reason");
					}
				}

				msgobj.deleteContent();

				auto_ptr<jsonInt_t> fp(new jsonInt_t((intL_t)0));
				msgobj.insertVal("position", fp.get());

				File::Size fpos = 0;
				unsigned int fragmentcnt = 0;
				do {
					fp->setVal(fpos);
					string msg = msgobj.toString();
					strcpy(data->data, msg.c_str());
					data->size = mFile.read(data->data + msg.size() + 1, DMAXSIZE - 1 - msg.size());
					fpos += data->size;
					mBytes += data->size;
					data->size += msg.size() + 1;

					if(!client.sendData(data.get())) throw runtime_error("Can't send!");
/*					if(!(fragmentcnt = (fragmentcnt +1) % 5000)) {
						bcastProgressUpdate(*this);
						if(outputpercentage) {
							printf("%ld\n", 100*mBytes / mSize);
							fflush(stdout);
						}
					}*/
				} while(fpos < mSize);
				strcpy(data->data, "{\"action\":\"finished\"}");
				data->size = 26;
				client.sendData(data.get());
				/*
				data->size = DMAXSIZE;
				client.recvData(data.get());
				data->data[data->size] = 0;
				string header(data->data);
				jsonObj_t h(&header);
				if(dynamic_cast<jsonStr_t &>(h["action"]).getVal() == "finished")
				*/
				//bcastProgressUpdate(*this);
				mReporter.join();
				mStatus = IS_TRANSFER_FINISHED;
				bcastProgressEnd(*this);
				return 1;
			} catch(...) {
				mStatus = IS_TRANSFER_ERROR;
				bcastProgressEnd(*this);
				throw;
			}
		}

		void fail() {
			mStatus = IS_TRANSFER_ERROR;
			bcastProgressEnd(*this);
		}

		~FileReader() {}

	private:
		string mName, mTarget;
		File mFile;
		File::Size mSize, mBytes;
		int mId, mStatus;
		StatusReporter &mReporter;
};

fileStatus_t::~fileStatus_t() {}

pluginInstanceAutoPtr<peer_t> findWay(jsonArr_t &ways) {
	pluginList_t &pl = pluginList_t::instance();
	for(int i = 0; i < ways.count(); ++i) {
		string pname = "UNKNOWN";
		try {
			// Prepare plugin configuration
			jsonObj_t &way = dynamic_cast<jsonObj_t &>(ways[i]);
			pname = dynamic_cast<jsonStr_t &>(way.gie("plugin")).getVal();

			// Load plugin and try to connect
			fprintf(stderr, "Connecting...\n");
			pluginInstanceAutoPtr<peer_t> client(pl[pname].as<connectionCreator_t>()->newClient(way.gie("config")));
			if(client.valid()) return client; // else fprintf(stderr, "Couldn't connect with way %d: %s\n", i, pl[pname].lastError());
		}
		catch(exception &e) {
			printf("Skipping %s: %s\n", pname.c_str(), e.what());
		}
		catch(...) {
			// continue trying
		}
	}
	throw runtime_error("No usable way found");
}

bool sendEntry(const string &entry, size_t ignored, const string &tname, peer_t &client) {
	if(entry.size() > 1 && entry.substr(entry.size() - 2) == "/.") return true;
	if(entry.size() > 2 && entry.substr(entry.size() - 3) == "/..") return true;
	bool success = true;
	try {
		Directory dir(entry);
		while(1) {
			success = success && sendEntry(combinePath(entry, dir.next()), ignored, tname, client);
		}
	}
	catch(ENotDir &e) {
		// Open file
		FileReader file(entry, tname);

		// Send file
		if(file.send(client, entry.substr(ignored))) {
			printf("File \"%s\" sent to \"%s\".\n", entry.substr(ignored).c_str(), tname.c_str());
			return true;
		} else success = false;
	}
	catch(Eod &e) {} // Ignore end of directory
	return success;
}

int main(int argc, char **argv) {
	if(argc < 2) return 1;

	string userdir(getUserDir());
	string cfgfile(combinePath(userdir, string("client.cfg")));

	// Find out if config file was specified
	for(int i = 1; i+1 < argc; ++i) {
		if(string(argv[i]) == "-c") {
			if(!argv[++i]) {
				fprintf(stderr, "Too few arguments after '-c'\n");
				return 1;
			}
			cfgfile = string(argv[i]);
			continue;
		} else
		if(string(argv[i]) == "-p") outputpercentage = 1;
	}
	int failuredetected = 0;

	try {
		pluginList_t &pl = pluginList_t::instance();
		pl.addSearchPath(getSystemPluginDir());
		pl.addSearchPath(combinePath(userdir, string("plugins")));

		// Load configuration
		auto_ptr<jsonComponent_t> configptr(cfgReadFile(cfgfile.c_str()));
		jsonObj_t &config = dynamic_cast<jsonObj_t &>(*configptr.get());
		jsonObj_t &targets = dynamic_cast<jsonObj_t &>(config.gie("targets"));

		try {
			eventSink_t::instance().autoLoad(dynamic_cast<jsonObj_t &>(config.gie("eventhandlers")));
		}
		catch(...) {
			fprintf(stderr, "Note: no event handler loaded\n");
		}           

		char *tname = NULL;
		pluginInstanceAutoPtr<peer_t> client;
		for(int i = 1; i+1 < argc; ++i) {
			if(string(argv[i]) == string("-t")) {
				if(!argv[++i]) {
					fprintf(stderr, "Too few arguments after '-t'\n");
					return 1;
				}
				tname = argv[i];
				try {
					client = findWay(dynamic_cast<jsonArr_t &>(dynamic_cast<jsonObj_t &>(targets.gie(tname)).gie("ways")));
				} catch(exception &e) {
					printf("Failed to connet to %s: %s\n", tname, e.what());
					fflush(stdout);
					failuredetected = 1; 
					while(i + 1 < argc && string(argv[i]) != string("-t")) ++i; // skip unusable target
					if(i + 1 < argc) --i;
					continue;
				}
				if(!client.valid()) {
					printf("Failed to connet to %s\n", tname);
					fflush(stdout);
					failuredetected = 1;
					while(i + 1 < argc && string(argv[i]) != string("-t")) ++i; // skip unusable target
					if(i + 1 < argc) --i;
					continue;
				}

				continue;
			}

			if(string(argv[i]) == string("-f") && tname) {
				if(!argv[++i]) {
					fprintf(stderr, "Too few arguments after '-f'\n");
					return 1;
				}
				
				string fn(argv[i]);
				trimSlashes(fn);
				
				fprintf(stderr, "Sending data...\n");
				if(sendEntry(fn, getFileName(fn), tname, *client)) {
					fprintf(stderr, "Sending successfull.\n");
				} else failuredetected = 1;
			}
		}

	}
	catch(const char *msg) {
		//char *dlerr = dlerror();
		fprintf(stderr, "Error: %s\n", msg);
		//if(dlerr) fprintf(stderr, "; %s\n", dlerr); else putc('\n', stderr);
		return 1;
	}
	fprintf(stderr, "Return value: %d\n", failuredetected);
	_exit(failuredetected);
}
