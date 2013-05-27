#include "filewriter.h"

class serverFileList_t : public fileList_t {
	private:
		fileController_t *insertController(int id, const string &fileName, File::Size fileSize, const string &machineID);
}
