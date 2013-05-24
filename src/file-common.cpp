#include "file.h"

const char *ENoSpace::what() const throw() { return "No space left on device"; }
ENoSpace::~ENoSpace() throw() {}

const char *ENotExist::what() const throw() { return "No such file or directory"; }
ENotExist::~ENotExist() throw() {}

const char *EPerm::what() const throw() { return "Permission denied"; }
EPerm::~EPerm() throw() {}

const char *EFileError::what() const throw() { return mDescription.c_str(); }
EFileError::~EFileError() throw() {}

const File &File::operator=(const File &file) {
	if(mData == file.mData) return file;
	++*file.mData;
	unref();
	mData = file.mData;
	return file;
}

File::Data::~Data() {}
