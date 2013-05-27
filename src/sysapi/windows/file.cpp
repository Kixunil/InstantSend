#include <windows.h>

#include "file.h"

using namespace std;

class WindowsFile : public File::Data {
	public:
		WindowsFile(const string &filename, bool writeable) : 
			mHandle(CreateFile(filename.c_str(), GENERIC_READ | (writeable?GENERIC_WRITE:0), FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) {
				if(mHandle == INVALID_HANDLE_VALUE) 
					throw EFileError("Unknown open file error");
			}

		File::Size readData(char *data, File::Size size, File::Size position) {
			LARGE_INTEGER pos;
			pos.QuadPart = position;
			if(!SetFilePointerEx(mHandle, pos, NULL, FILE_BEGIN))
				throw EFileError("Unknown seek file error");
			DWORD bytes;
			if(!ReadFile(mHandle, data, (DWORD)size, &bytes, NULL))
				throw EFileError("Unknown read file error");
			return bytes;
		}

		File::Size readData(char *data, File::Size size) {
			DWORD bytes;
			if(!ReadFile(mHandle, data, (DWORD)size, &bytes, NULL))
				throw EFileError("Unknown read file error");
			return bytes;
		}

		void writeData(const char *data, File::Size size, File::Size position) {
			LARGE_INTEGER pos;
			pos.QuadPart = position;
			if(!SetFilePointerEx(mHandle, pos, NULL, FILE_BEGIN))
				throw EFileError("Unknown seek file error");
			DWORD tmp;
			if(!WriteFile(mHandle, data, (DWORD)size, &tmp, NULL))
				throw EFileError("Unknown write file error");
		}

		File::Size getSize() {
			LARGE_INTEGER result;
			if(!GetFileSizeEx(mHandle, &result)) 
				throw EFileError("Unknown get file size error");
			return result.QuadPart;
		}

		~WindowsFile() {
			CloseHandle(mHandle);
		}
	private:
		HANDLE mHandle;
};

File::File(const std::string &fileName) : mData(new WindowsFile(fileName, false)) {}

WritableFile::WritableFile(const std::string &fileName) : File(new WindowsFile(fileName, true)) {}
