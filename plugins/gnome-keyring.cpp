#include <gnome-keyring.h>

#include "pluginapi.h"

using namespace std;

namespace InstantSend {

const GnomeKeyringPasswordSchema genericSchema = {
	GNOME_KEYRING_ITEM_GENERIC_SECRET, {
		{ "name", GNOME_KEYRING_ATTRIBUTE_TYPE_STRING },
		{ NULL, GNOME_KEYRING_ATTRIBUTE_TYPE_STRING }
	}
};

class GnomeKeyring : public SecureStorage {
	public:
		inline GnomeKeyring(const string &keyringName) : mName(new string(keyringName)) {}
		inline GnomeKeyring() : mName(NULL) {}

		bool isLocked() {
			GnomeKeyringInfo *keyring_info;
			if(gnome_keyring_get_info_sync(keyringName(), &keyring_info) != GNOME_KEYRING_RESULT_OK) return true;
			bool result = gnome_keyring_info_get_is_locked(keyring_info);
			gnome_keyring_info_free(keyring_info);
			return result;
		}

		bool reuqestUnlock() {
			GnomeKeyringResult res = gnome_keyring_unlock_sync(keyringName(), NULL);
			return res == GNOME_KEYRING_RESULT_OK || res == GNOME_KEYRING_RESULT_ALREADY_UNLOCKED;
		}

		bool getSecret(const std::string &name, std::string &value, bool silent) {
			if(!(silent || reuqestUnlock())) return false;

			gchar *pass;

			if(gnome_keyring_find_password_sync(&genericSchema, &pass, "name", name.c_str(), NULL) != GNOME_KEYRING_RESULT_OK) return false;
			value = pass;

			gnome_keyring_free_password(pass);
			return true;
		}

		bool setSecret(const std::string &name, const std::string &value, bool silent) {
			if(!(silent || reuqestUnlock())) return false;

			return gnome_keyring_store_password_sync(&genericSchema, keyringName(), "InstantSend secret", value.c_str(), "name", name.c_str(), NULL) == GNOME_KEYRING_RESULT_OK;
		}

		bool removeSecret(const std::string &name) {
			return gnome_keyring_delete_password_sync(&genericSchema, "name", name.c_str(), NULL) == GNOME_KEYRING_RESULT_OK;
		}

		GnomeKeyring &operator=(const GnomeKeyring &other) {
			mName.reset(other.mName.get()?new string(*other.mName):NULL);
		}

		GnomeKeyring(const GnomeKeyring &other) : mName(other.mName.get()?new string(*other.mName):NULL) {}

	private:
		inline const gchar *keyringName() const {
			return mName.get()?mName->c_str():GNOME_KEYRING_DEFAULT;
		}

		auto_ptr<string> mName;
};

class GnomeKeyringCreator : public SecureStorageCreator {
	public:
		inline GnomeKeyringCreator(PluginEnvironment &pluginEnv) : SecureStorageCreator(pluginEnv) {}
		SecureStorage *openSecureStorage(const jsonComponent_t *config) {
			try {
				if(!config) throw runtime_error("No config");
				GnomeKeyring tmp(dynamic_cast<const jsonStr_t &>(dynamic_cast<const jsonObj_t &>(*config)["keyring"]).getVal());
				keyring = tmp;
			}
			catch(...) {
				keyring = GnomeKeyring();
			}

			return &keyring;
		}

		void closeSecureStorage(SecureStorage &) {}

	private:
		GnomeKeyring keyring;
};

}

extern "C" InstantSend::PluginInstanceCreator *getCreator(InstantSend::PluginEnvironment &env) {
	static InstantSend::GnomeKeyringCreator creator(env);
	return &creator;
}
