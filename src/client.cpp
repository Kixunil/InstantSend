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

using namespace InstantSend;
using namespace std;

int outputpercentage = 0;

const int maxProtocolVersion = 1;

inline size_t getBaseFileName(const string &path) {
	return getFileName(path);
}

class StatusReporter : public thread_t {
	public:
		StatusReporter(FileStatus &fs) : mFileStatus(fs), mRunning(true), mSem(0) {}
		void run() {
			while(mRunning) {
#ifndef WINDOWS
				usleep(500000);
#else
				_sleep(1);
#endif
				bcastProgressUpdate(mFileStatus);
				if(outputpercentage) {
					printf("%ld\n", 100*mFileStatus.getTransferredBytes() / mFileStatus.getFileSize());
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
		FileStatus &mFileStatus;
		volatile bool mRunning;
		Semaphore mSem;

};

class FileReader : public FileStatus {
	public:
		FileReader(const string &name, const string &target) : mName(name), mTarget(target), mFile(name), mSize(mFile.size()), mBytes(0),
#ifndef WINDOWS
		mId(getpid()),
#endif
		mStatus(IS_TRANSFER_CONNECTING)
#ifndef WINDOWS
		, mReporter(*new StatusReporter(*this))
#endif
	       	{
			bcastProgressBegin(*this);
#ifndef WINDOWS
			mReporter.start();
#endif
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

		int send(Peer &client, const string &basename) {
			try {
				LOG(Logger::Debug, "File size: %llu", (unsigned long long)mSize);

				mStatus = IS_TRANSFER_IN_PROGRESS;

				bcastProgressUpdate(*this);

				auto_ptr<anyData> data(allocData(DMAXSIZE+1));

				jsonObj_t msgobj;
				msgobj.insertNew("service", new jsonStr_t("filetransfer"));
				msgobj.insertNew("filename", new jsonStr_t(basename.c_str()));
				msgobj.insertNew("filesize", new jsonInt_t((intL_t)mSize));
				msgobj.insertNew("max_msg_size", new jsonInt_t((intL_t)DMAXSIZE));
				msgobj.insertNew("version", new jsonInt_t((intL_t)maxProtocolVersion));
				string msg = msgobj.toString();
				strcpy(data->data, msg.c_str());
				data->size = msg.size() + 1;
				//msgobj.deleteContent();

				client.sendData(data.get());
				if(!client.sendData(data.get())) throw runtime_error("Can't send!");
				data->size = DMAXSIZE;
				if(!client.recvData(data.get())) throw runtime_error("Receiving failed");
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

				try {
					mPeerMaxSize = dynamic_cast<jsonInt_t &>(msgobj["max_msg_size"]).getVal();
				} catch(...) {
					mPeerMaxSize = OLD_DMAXSIZE;
				}

				LOG(Logger::Debug, "Max datagram size of peer = %d", (int)mPeerMaxSize);
				int protocolVersion;
				try {
					protocolVersion = dynamic_cast<jsonInt_t &>(msgobj["version"]).getVal();
				} catch(...) {
					protocolVersion = 0;
				}

				LOG(Logger::Debug, "Using protocol version %d", protocolVersion);

				if(protocolVersion > maxProtocolVersion) {
					protocolVersion = maxProtocolVersion;
				}

				msgobj.deleteContent();

				auto_ptr<jsonInt_t> fp(new jsonInt_t((intL_t)0));
				msgobj.insertVal("position", fp.get());

				File::Size fpos = 0;
				unsigned int fragmentcnt = 0;
				do {
					size_t dataOffset = 0;
					if(protocolVersion < 1) {
						fp->setVal(fpos);
						string msg = msgobj.toString();
						strcpy(data->data, msg.c_str());
						dataOffset += msg.size() + 1;
					} else {
						File::Size tmp(fpos);
						data->data[dataOffset] = 0;
						++dataOffset;
						for(size_t i = dataOffset + 7; i >= dataOffset; --i) {
							data->data[i] = tmp % 256;
							tmp /= 256;
						}
						dataOffset += 8;
					}

					size_t readSize = mFile.read(data->data + dataOffset, mPeerMaxSize - dataOffset);
					data->size = dataOffset + readSize;
					if(!client.sendData(data.get())) throw runtime_error("Can't send!");
					fpos += readSize;
					mBytes += readSize;

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
#ifndef WINDOWS
				mReporter.join();
#endif
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
		size_t mPeerMaxSize;
#ifndef WINDOWS
		StatusReporter &mReporter;
#endif
};

FileStatus::~FileStatus() {}

pluginInstanceAutoPtr<Peer> findWay(jsonArr_t &ways) {
	PluginList &pl = PluginList::instance();
	for(int i = 0; i < ways.count(); ++i) {
		string pname = "UNKNOWN";
		try {
			// Prepare plugin configuration
			jsonObj_t &way = dynamic_cast<jsonObj_t &>(ways[i]);
			pname = dynamic_cast<jsonStr_t &>(way.gie("plugin")).getVal();

			// Load plugin and try to connect
			LOG(Logger::Debug, "Connecting...");
			pluginInstanceAutoPtr<Peer> client(pl[pname].as<ConnectionCreator>()->newClient(way.gie("config")));
			if(client.valid()) return client;
		}
		catch(exception &e) {
			LOG(Logger::Note, "Skipping %s: %s", pname.c_str(), e.what());
		}
		catch(...) {
			// continue trying
		}
	}
	throw runtime_error("No usable way found");
}

bool sendEntry(const string &entry, size_t ignored, const string &tname, Peer &client) {
	LOG(Logger::Debug, "Sending %s", entry.c_str());
	if(entry.size() > 1 && (entry.substr(entry.size() - 2) == "/." || entry.substr(entry.size() - 2) == "\\.")) return true;
	if(entry.size() > 2 && (entry.substr(entry.size() - 3) == "/.."|| entry.substr(entry.size() - 3) == "\\..")) return true;
	bool success = true;
	try {
		Directory dir(entry);
		LOG(Logger::Debug, "Sending directory %s", entry.c_str());
		while(1) {
			success = success && sendEntry(combinePath(entry, dir.next()), ignored, tname, client);
		}
	}
	catch(ENotDir &e) {
		LOG(Logger::Debug, "Opening %s", entry.c_str());
		// Open file
		FileReader file(entry, tname);

		// Send file
		if(file.send(client, entry.substr(ignored))) {
			LOG(Logger::Note, "File \"%s\" sent to \"%s\".", entry.substr(ignored).c_str(), tname.c_str());
			return true;
		} else success = false;
	}
	catch(Eod &e) {} // Ignore end of directory
	return success;
}

int main(int argc, char **argv) {
	auto_ptr<Application> app(new Application(argc, argv));
	instantSend = app.get();

	if(argc < 2) return 1;

	string userdir(getUserDir());
	string cfgfile(combinePath(userdir, string("client.cfg")));

	// Find out if config file was specified
	for(int i = 1; i+1 < argc; ++i) {
		if(string(argv[i]) == "-c") {
			if(!argv[++i]) {
				LOG(Logger::Error, "Too few arguments after '-c'");
				return 1;
			}
			cfgfile = string(argv[i]);
			continue;
		} else
		if(string(argv[i]) == "-p") outputpercentage = 1;
	}
	int failuredetected = 0;

	try {
		PluginList &pl = PluginList::instance();
		pl.addSearchPath(getSystemPluginDir());
		pl.addSearchPath(combinePath(userdir, string("plugins")));

		// Load configuration
		auto_ptr<jsonComponent_t> configptr(cfgReadFile(cfgfile.c_str()));
		jsonObj_t &config = dynamic_cast<jsonObj_t &>(*configptr.get());
		jsonObj_t &targets = dynamic_cast<jsonObj_t &>(config.gie("targets"));

		try {
			EventSink::instance().autoLoad(dynamic_cast<jsonObj_t &>(config.gie("eventhandlers")));
		}
		catch(...) {
			LOG(Logger::Note, "No event handler loaded");
		}           

		char *tname = NULL;
		pluginInstanceAutoPtr<Peer> client;
		for(int i = 1; i+1 < argc; ++i) {
			if(string(argv[i]) == string("-t")) {
				if(!argv[++i]) {
					LOG(Logger::Error, "Too few arguments after '-t'");
					return 1;
				}
				tname = argv[i];
				try {
					client = findWay(dynamic_cast<jsonArr_t &>(dynamic_cast<jsonObj_t &>(targets.gie(tname)).gie("ways")));
				} catch(exception &e) {
					LOG(Logger::Error, "Failed to connet to %s: %s", tname, e.what());
					failuredetected = 1; 
					while(i + 1 < argc && string(argv[i]) != string("-t")) ++i; // skip unusable target
					if(i + 1 < argc) --i;
					continue;
				}
				if(!client.valid()) {
					LOG(Logger::Error, "Failed to connet to %s", tname);
					failuredetected = 1;
					while(i + 1 < argc && string(argv[i]) != string("-t")) ++i; // skip unusable target
					if(i + 1 < argc) --i;
					continue;
				}

				continue;
			}

			if(string(argv[i]) == string("-f") && tname) {
				if(!argv[++i]) {
					LOG(Logger::Error, "Too few arguments after '-f'");
					return 1;
				}
				
				string fn(argv[i]);
				trimSlashes(fn);
				
				LOG(Logger::Debug, "Sending data...");
				if(sendEntry(fn, getFileName(fn), tname, *client)) {
					LOG(Logger::Debug, "Sending successfull.");
				} else failuredetected = 1;
			}
		}

	}
	catch(const char *msg) {
		//char *dlerr = dlerror();
		LOG(Logger::Error, "Error: %s", msg);
		return 1;
	}
	catch(exception &e) {
		LOG(Logger::Error, "Error: %s", e.what());
		return 1;
	}
	LOG(Logger::Debug, "Return value: %d", failuredetected);
	_exit(failuredetected);
}
