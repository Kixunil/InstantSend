#include "pluginapi.h"

namespace InstantSend {

	/*! Used to prevent naming conflicts */
class SecureStorageWrapper : public SecureStorage {
	public:
		inline SecureStorageWrapper(const std::string &prefix, SecureStorage &secureStorage) : mPrefix(prefix), mSecureStorage(secureStorage) {}
		bool isLocked();
		bool reuqestUnlock();
		bool getSecret(const std::string &name, std::string &value, bool silent = false);
		bool setSecret(const std::string &name, const std::string &value, bool silent = false);
		bool removeSecret(const std::string &name);		
	private:
		std::string mPrefix;
		SecureStorage &mSecureStorage;
};

}
