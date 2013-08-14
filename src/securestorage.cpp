#include "securestorage.h"

using namespace std;

bool InstantSend::SecureStorageWrapper::isLocked() {
	return mSecureStorage.isLocked();
}

bool InstantSend::SecureStorageWrapper::reuqestUnlock() {
	return mSecureStorage.reuqestUnlock();
}

bool InstantSend::SecureStorageWrapper::getSecret(const std::string &name, std::string &value, bool silent) {
	return mSecureStorage.getSecret(mPrefix + name, value, silent);
}

bool InstantSend::SecureStorageWrapper::setSecret(const std::string &name, const std::string &value, bool silent) {
	return mSecureStorage.setSecret(mPrefix + name, value, silent);
}

bool InstantSend::SecureStorageWrapper::removeSecret(const std::string &name) {
	return mSecureStorage.removeSecret(mPrefix + name);
}
