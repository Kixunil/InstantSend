#ifndef FILELIST
#define FILELIST

#include <map>

#include "multithread.h"
#include "pluginapi.h"

namespace InstantSend {

class fileController_t : public FileStatus {
	private:
		int identifier;
		volatile int refcount;
		Mutex refmutex;
		Semaphore refSem;
	protected:
		virtual void zeroReferences() = 0;
		void waitZR();
	public:
		fileController_t(int id);
		void incRC();
		void decRC();
		int getId();
		virtual ~fileController_t();
};

class fileList_t {
	protected:
		map<int, fileController_t *> identifiers;
		fileController_t *insertController(int id, const std::string &fileName, File::Size fileSize, const std::string &machineID, jsonObj_t* extras);
		Mutex listmutex;
	public:
		static fileList_t &getList();
		fileController_t &getController(int id, const std::string &fileName, File::Size, const std::string &machineID, jsonObj_t* extras);
		void removeController(int id, bool del);
};

}

#endif
