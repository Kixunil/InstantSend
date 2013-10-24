#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <cstring>

#include <cstdio>
#include <dirent.h>

#include "file.h"

#ifdef __APPLE__
	#define O_LARGEFILE 0
	#define lseek64 lseek
#endif

#define MKSTR(NUM) #NUM
#define TOSTRING(ARG) MKSTR(ARG)
#define POS (__FILE__ ":" TOSTRING(__LINE__))

using namespace std;

class EAutoFile: public EFileError {
	public:
		EAutoFile(const string &prefix) : EFileError(prefix + ": " + strerror(errno)) {}
		~EAutoFile() throw() {}
};

class PosixFile : public File::Data {
	public:
		inline PosixFile(int fd) : mFd(fd) {}
		PosixFile(const string &filename, bool writeable) : mFd(writeable?open(filename.c_str(), O_RDWR | O_LARGEFILE | O_CREAT, S_IRUSR | S_IWUSR):open(filename.c_str(), O_RDONLY | O_LARGEFILE)) {
			//fprintf(stderr, "Opening %s\n", filename.c_str());
			if(mFd < 0) throw EAutoFile("open");
		}

		File::Size readData(char *data, File::Size size, File::Size position) {
			if(lseek64(mFd, position, SEEK_SET) == -1) throw EAutoFile(string(POS) + ": lseek64");
			File::Size result = read(mFd, data, size);
			if(result == -1) throw EAutoFile(string(POS) + ": read");
			return result;
		}

		File::Size readData(char *data, File::Size size) {
			File::Size result = read(mFd, data, size);
			if(result == -1) throw EAutoFile(string(POS) + ": read");
			return result;
		}

		void writeData(const char *data, File::Size size, File::Size position) {
			if(lseek64(mFd, position, SEEK_SET) == -1) throw EAutoFile(string(POS) + ": lseek64");
			while(size) {
				ssize_t len = write(mFd, data, size);
				if(len < 0) switch(errno) {
					case ENOSPC:
						throw ENoSpace();
					default:
						throw EAutoFile(string(POS) + ": write");
				}
				size -= len;
				data += len;
			}
		}

		File::Size getSize() {
			// Remember current position
			File::Size oldpos = lseek64(mFd, 0, SEEK_CUR);
			if(oldpos == -1) throw EAutoFile(string(POS) + ": lseek64");
			// Seek to end, to find out file size
			File::Size result = lseek64(mFd, 0, SEEK_END);
			if(result == -1) throw EAutoFile(string(POS) + ": lseek64");
			// Return to previous position
			if(lseek64(mFd, oldpos, SEEK_SET) == -1) throw EAutoFile(string(POS) + ": lseek64");

			return result;
		}

		~PosixFile() {
			close(mFd);
		}
	private:
		int mFd;
};

File::File(const std::string &fileName) : mData(new PosixFile(fileName, false)) {}

WritableFile::WritableFile(const std::string &fileName) : File(new PosixFile(fileName, true)) {}

class PosixDirectory : public Directory::Data {
	public:
		PosixDirectory(const string &path) : Directory::Data(), mDir(opendir(path.c_str())) {
			if(!mDir) {
				if(errno == ENOTDIR) throw ENotDir();
				else throw EFileError(string("opendir: ") + strerror(errno));
			}
		}
		string next() {
			struct dirent *de = readdir(mDir);
			if(!de) throw Eod();
			return string(de->d_name);
		}

		void rewind() {
			rewinddir(mDir);
		}

		~PosixDirectory() {
			closedir(mDir);
		}
	private:
		DIR *mDir;
};

Directory::Directory(const string &dir) : mData(new PosixDirectory(dir)) {}
