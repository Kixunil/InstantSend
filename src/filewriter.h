#ifndef FILEWRITER
#define FILEWRITER

#include <queue>
#include <set>
#include <memory>

#include "pluginapi.h"
#include "multithread.h"
#include "filelist.h"

#define MAX_BUF_FRAGMENT_COUNT 32

namespace InstantSend {

class dataFragment_t {
	private:
		anyData *dat;
		File::Size pos;
		int batch;
	public:
		dataFragment_t(auto_ptr<anyData> &data, File::Size position, int batchNumber);
		bool writeData(WritableFile &file) const;
		bool operator<(const dataFragment_t &df) const;
		inline size_t size() const {
			return dat->size;
		}
		inline void freeData() const {
			delete dat;
		}
};

class fileWriter_t : public fileController_t, thread_t {
	protected:
		std::string fName; 
		File::Size fSize; 
		std::string mId;

		WritableFile file;
		Mutex mutex;
		Semaphore inputSem, outputSem;
		priority_queue<dataFragment_t> queue;
		bool empty();

		File::Size lastpos;
		int lastBatch;

		File::Size bytes;

		bool hardPause;
		bool stop, hasStopped;
		bool zr;

		int tStatus;

		unsigned int lastUpdate, updateInterval;

		void zeroReferences();
		bool cleanupCheck(bool calledByThisThread = false);
		void writeBuffer();

		std::auto_ptr<jsonObj_t> mExtras;
	public:
		fileWriter_t(int id, const std::string &fileName, File::Size fileSize, const std::string &machineId, jsonObj_t *extras);
		std::string getFileName();
		File::Size getFileSize();
		std::string getMachineId();

		bool writeData(File::Size position, auto_ptr<anyData> &data);
		void run();
		bool autoDelete();
		File::Size getTransferredBytes();
		int getTransferStatus();
		char getDirection();
		const jsonObj_t *getExtras();

		void pauseTransfer(); 
		void resumeTransfer();

		~fileWriter_t();
};

class FileWriterPtr {
	public:
		inline FileWriterPtr(fileWriter_t &fw) : mFW(&fw) { mFW->incRC(); }
		inline FileWriterPtr(const FileWriterPtr &other) : mFW(other.mFW) { mFW->incRC(); }
		inline FileWriterPtr &operator=(const FileWriterPtr &other) {
			other.mFW->incRC();
			mFW->decRC();
			mFW = other.mFW;
			return *this;
		}
		inline ~FileWriterPtr() { mFW->decRC(); }
		inline fileWriter_t &operator*() {
			return *mFW;
		}

		inline fileWriter_t *operator->() {
			return mFW;
		}
	private:
		fileWriter_t *mFW;
};

}

#endif
