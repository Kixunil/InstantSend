#ifdef WINDOWS
#include <windows.h>
#else
	#ifdef __APPLE__
		#define off64_t off_t
	#else
		#include <sys/stat.h>
	#endif
#endif

#include <string>

class ENoSpace : public std::exception {
	public:
		const char *what() const throw();
		~ENoSpace() throw();
};

class ENotExist : public std::exception {
	public:
		const char *what() const throw();
		~ENotExist() throw();
};

class ENotDir : public std::exception {
	public:
		const char *what() const throw();
		~ENotDir() throw();            
};

class EPerm : public std::exception {
	public:
		const char *what() const throw();
		~EPerm() throw();
};

class EFileError : public std::exception {
	public:
		inline EFileError(const std::string &description) : mDescription(description) {}
		const char *what() const throw();
		~EFileError() throw();
	private:
		std::string mDescription;
};

class Eod {};

class File {
	public:
		// These should be practically same but just in case...
#ifdef WINDOWS
		typedef __int64 Size;
#else
		typedef off64_t Size;
#endif

		class Data {
			public:
				inline Data() : mRefs(1) {}
				virtual File::Size readData(char *data, File::Size size, File::Size position) = 0;
				virtual File::Size readData(char *data, File::Size size) = 0;
				virtual void writeData(const char *data, File::Size size, File::Size position) = 0;
				virtual File::Size getSize() = 0;
				virtual ~Data();
				inline unsigned int operator++() { return ++mRefs; }
				inline unsigned int operator--() { return --mRefs; }
			private:
				volatile unsigned int mRefs;
		};

		File(const std::string &fileName);
		inline File(const File &file) : mData(file.mData) { ++*mData; }
		const File &operator=(const File &file);
		inline ~File() {
			unref();
		}

		inline File::Size read(char *data, File::Size size, File::Size position) { return mData->readData(data, size, position); }
		inline File::Size read(char *data, File::Size size) { return mData->readData(data, size); }

		inline File::Size size() { return mData->getSize(); }

	protected:
		inline File(File::Data *data) : mData(data) {}
		inline void unref() { if(!--*mData) delete mData; }
		File::Data *mData;
};

class WritableFile : public File {
	public:
		WritableFile(const std::string &fileName);
		inline void write(const char *data, File::Size size, File::Size position) { static_cast<WritableFile::Data &>(*mData).writeData(data, size, position); }
};

class Directory {
	public:
		class Data {
			public:
				inline Data() : mRefs(1) {}
				virtual std::string next() = 0;
				virtual void rewind() = 0;
				virtual ~Data();
				inline unsigned int operator++() { return ++mRefs; }
				inline unsigned int operator--() { return --mRefs; }
			private:
				volatile unsigned int mRefs;
		};

		Directory(const std::string &dir);
		inline Directory(const Directory &directory) : mData(directory.mData) { ++*mData; }
		Directory &operator=(const Directory &dir);
		inline ~Directory() { unref(); }

		inline std::string next() { return mData->next(); }
		inline void rewind() { mData->next(); }
	protected:
		inline Directory(Directory::Data *data) : mData(data) {}
		inline void unref() { if(!--*mData) delete mData; }
		Directory::Data *mData;
};
