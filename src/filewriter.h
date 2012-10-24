#include <queue>
#include <set>
#include <memory>

#include "pluginapi.h"
#include "multithread.h"
#include "filelist.h"

#define MAXBUFSIZE 65536

using namespace std;

class dataFragment_t {
	private:
		anyData *dat;
		long pos;
		int batch;
	public:
		dataFragment_t(auto_ptr<anyData> &data, long position, int batchNumber);
		bool writeData(FILE *file) const;
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
		string fName; 
		size_t fSize; 
		string mId;

		FILE *f;
		auto_ptr<mutex_t> mutex;
		priority_queue<dataFragment_t> queue;
		bool empty();

		long lastpos;
		int lastBatch;

		size_t bytes;
		size_t bufsiz;
		set<thread_t *> waitingThreads;

		bool hardPause;
		bool stop;
		bool zr;

		int tStatus;

		unsigned int lastUpdate, updateInterval;

		void zeroReferences();
		bool cleanupCheck(bool calledByThisThread = false);
		void writeBuffer();
		bool shouldRun();
	public:
		fileWriter_t(int id, const string &fileName, size_t fileSize, const string &machineId);
		string getFileName();
		size_t getFileSize();
		string getMachineId();

		bool writeData(long position, auto_ptr<anyData> &data, thread_t &caller);
		void run();
		bool autoDelete();
		size_t getTransferredBytes();
		int getTransferStatus();
		char getDirection();

		void pauseTransfer(); 
		void resumeTransfer();

		~fileWriter_t();
};
