#include <mate-keyring.h>

#include "pluginapi.h"

using namespace std;

namespace InstantSend {

const MateKeyringPasswordSchema genericSchema = {
	MATE_KEYRING_ITEM_GENERIC_SECRET, {
		{ "name", MATE_KEYRING_ATTRIBUTE_TYPE_STRING },
		{ NULL, MATE_KEYRING_ATTRIBUTE_TYPE_STRING }
	}
};

class MateKeyring : public SecureStorage {
	public:
		inline MateKeyring(const string &keyringName) : mName(new string(keyringName)) {}
		inline MateKeyring() : mName(NULL) {}

		bool isLocked() {
			MateKeyringInfo *keyring_info;
			if(mate_keyring_get_info_sync(keyringName(), &keyring_info) != MATE_KEYRING_RESULT_OK) return true;
			bool result = mate_keyring_info_get_is_locked(keyring_info);
			mate_keyring_info_free(keyring_info);
			return result;
		}

		bool reuqestUnlock() {
			MateKeyringResult res = mate_keyring_unlock_sync(keyringName(), NULL);
			return res == MATE_KEYRING_RESULT_OK || res == MATE_KEYRING_RESULT_ALREADY_UNLOCKED;
		}

		bool getSecret(const std::string &name, std::string &value, bool silent) {
			if(!(silent || reuqestUnlock())) return false;

			gchar *pass;

			if(mate_keyring_find_password_sync(&genericSchema, &pass, "name", name.c_str(), NULL) != MATE_KEYRING_RESULT_OK) return false;
			value = pass;

			mate_keyring_free_password(pass);
			return true;
		}

		bool setSecret(const std::string &name, const std::string &value, bool silent) {
			if(!(silent || reuqestUnlock())) return false;

			return mate_keyring_store_password_sync(&genericSchema, keyringName(), "InstantSend secret", value.c_str(), "name", name.c_str(), NULL) == MATE_KEYRING_RESULT_OK;
		}

		bool removeSecret(const std::string &name) {
			return mate_keyring_delete_password_sync(&genericSchema, "name", name.c_str(), NULL) == MATE_KEYRING_RESULT_OK;
		}

		MateKeyring &operator=(const MateKeyring &other) {
			mName.reset(other.mName.get()?new string(*other.mName):NULL);
		}

		MateKeyring(const MateKeyring &other) : mName(other.mName.get()?new string(*other.mName):NULL) {}

	private:
		inline const gchar *keyringName() const {
			return mName.get()?mName->c_str():MATE_KEYRING_DEFAULT;
		}

		auto_ptr<string> mName;
};

class MateKeyringCreator : public SecureStorageCreator {
	public:
		inline MateKeyringCreator(PluginEnvironment &pluginEnv) : SecureStorageCreator(pluginEnv) {}
		SecureStorage *openSecureStorage(const jsonComponent_t *config) {
			try {
				if(!config) throw runtime_error("No config");
				MateKeyring tmp(dynamic_cast<const jsonStr_t &>(dynamic_cast<const jsonObj_t &>(*config)["keyring"]).getVal());
				keyring = tmp;
			}
			catch(...) {
				keyring = MateKeyring();
			}

			return &keyring;
		}

		void closeSecureStorage(SecureStorage &) {}

	private:
		MateKeyring keyring;
};

}

extern "C" InstantSend::PluginInstanceCreator *getCreator(InstantSend::PluginEnvironment &env) {
	static InstantSend::MateKeyringCreator creator(env);
	return &creator;
}
