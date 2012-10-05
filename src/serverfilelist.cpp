#include "serverfilelist.h"

fileList_t::getList() {
	static auto_ptr<fileList_t> list(new serverFileList_t());
	return *list.get();
}

fileController_t *serverFileList_t::insertController(int id, const string &fileName, size_t fileSize, const string &machineID) {
	auto_ptr<fileController_t> controller(new fileWriter(id, fileName, fileSize, machineID));
	identifiers.insert(pair<int, fileController_t *(id, controller.get())>);
	return controller.release();
}
