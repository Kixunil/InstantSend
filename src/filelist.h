#include <map>

#include "multithread.h"
#include "pluginapi.h"

class fileController_t : public fileStatus_t {
	private:
		int identifier;
		int refcount;
		auto_ptr<mutex_t> refmutex;
	protected:
		virtual void zeroReferences() = 0;
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
		fileController_t *insertController(int id, const string &fileName, size_t fileSize, const string &machineID);
		auto_ptr<mutex_t> listmutex;
	public:
		inline fileList_t() : listmutex(mutex_t::getNew()) {}
		static fileList_t &getList();
		fileController_t &getController(int id, const string &fileName, size_t fileSize, const string &machineID);
		void removeController(int id, bool del);
};
