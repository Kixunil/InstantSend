#include "filewriter.h"

class serverFileList_t : public fileList_t {
	private:
		fileController_t *insertController(int id, const string &fileName, size_t fileSize, const string &machineID);
}
