#ifndef PLUGIN_H
#define PLUGIN_H
#include <set>
#include <string>
#include <stdexcept>
#include <assert.h>
#include <cstdio>

#include "pluginapi.h"
#include "securestorage.h"
#include "appcontrol.h"

namespace InstantSend {

template <class T>
struct pluginInstanceAutoPtrRef {
	T *mInstance;
	pluginInstanceAutoPtrRef(T *instance) : mInstance(instance) {}
};

typedef unsigned int RefCnt;

class PluginStorageHandle {
	public:
		virtual void checkUnload() = 0;
		virtual ~PluginStorageHandle();
};

class InternalPluginEnvironment : public PluginEnvironment {
	public:
		InternalPluginEnvironment(Application &app, const std::string &name);

		const std::string &systemPluginDataDir();
		const std::string &systemPluginConfigDir();
		const std::string &userPluginDir();

		void onInstanceCreated();
		void onInstanceDestroyed();

		void log(Logger::Level level, const std::string &message);

		SecureStorage *secureStorage();

		void checkUnload();
		inline void setStorageHandle(PluginStorageHandle *handle) { mStorageHandle.reset(handle); }

	private:
		const std::string mName;
		RefCnt instanceCount;
		std::auto_ptr<PluginStorageHandle> mStorageHandle;
		std::auto_ptr<SecureStorageWrapper> mSecureStorage;
};

template <class T>
class pluginInstanceAutoPtr {
	public:
		inline pluginInstanceAutoPtr(T *instance = NULL) throw() : mInstance(instance) {}
		inline pluginInstanceAutoPtr(std::auto_ptr<T> instance) throw() : mInstance(instance.release()) {}
		inline ~pluginInstanceAutoPtr() { reset(); }
		inline pluginInstanceAutoPtr(pluginInstanceAutoPtr &other) throw() : mInstance(other.release()) {}
		inline pluginInstanceAutoPtr(const pluginInstanceAutoPtrRef<T> &other) throw() : mInstance(other.mInstance) {}
		inline pluginInstanceAutoPtr &operator=(pluginInstanceAutoPtr &other) throw() { reset(other.release()); return *this; }
		inline pluginInstanceAutoPtr &operator=(std::auto_ptr<T> &other) throw() { reset(other.release()); return *this; }
		inline pluginInstanceAutoPtr &operator=(const pluginInstanceAutoPtrRef<T> &other) throw() { reset(other.mInstance); return *this; }
		template<class T2>
			pluginInstanceAutoPtr &operator=(pluginInstanceAutoPtr<T2> &other) { reset(other.release()); return *this; }
		inline T &operator*() const throw() { return *mInstance; }
		inline T *operator->() const throw() { return mInstance; }
		inline operator pluginInstanceAutoPtrRef<T>() { return pluginInstanceAutoPtrRef<T>(this->release()); }
		template <class T2>
			inline operator pluginInstanceAutoPtrRef<T2>() { return pluginInstanceAutoPtrRef<T2>(this->release()); }
		void reset(T *instance = NULL) {
			if(mInstance) {
				InternalPluginEnvironment &env(static_cast<InternalPluginEnvironment &>(mInstance->mEnv));
				fprintf(stderr, "Plugin environment pointer = %p\n", &env);
				delete mInstance;
				env.checkUnload();
			}
			mInstance = instance;
		}
		inline bool valid() { return mInstance; }
	private:
		inline T *release() {
			T *tmp(mInstance);
			mInstance = NULL;
			return tmp;
		}
		T *mInstance;
};


class EPluginInvalid : public std::runtime_error {
	inline EPluginInvalid(const std::string &message) : std::runtime_error(message) {};
};

class LibraryHandle {
	public:
		virtual void onUnload() = 0;
		virtual ~LibraryHandle();
		inline PluginInstanceCreator *creator() { return mCreator; }
	protected:
		PluginInstanceCreator *mCreator;
};

/*! \brief Holds all necessary informations about plugin
 */
class pluginHandle_t {
	private:
		/*! Reference counter */
		unsigned int refCnt;

		/*! Handle to dynamic library */
		LibraryHandle *mLibrary;

		bool mOwner;
	public:
		/*! Standard constructor */
		inline pluginHandle_t() : refCnt(0), mLibrary(NULL), mOwner(false) {}
		pluginHandle_t(std::auto_ptr<LibraryHandle> library);
		pluginHandle_t(const pluginHandle_t &other);

		/* Frees associated resources and unloads library */
		~pluginHandle_t();

		inline void assignLibrary(std::auto_ptr<LibraryHandle> library) {
			mLibrary = library.release();
			mOwner = true;
		}

		/*! Increases reference count */
		inline void incRC() throw() {
			++refCnt;
		}

		/*! Decreases reference count and returns true, if ti's possible to unload plugin */
		inline bool decRC() throw() {
			return !--refCnt;
		}

		/*! Returns actual reference count */
		inline unsigned int getRC() {
			return refCnt;
		}

		inline bool isUnloadable() {
			return !refCnt;
		}

		/* Returns pointer to instance creator */
		inline PluginInstanceCreator *creator() {
			return mLibrary->creator();
		}

		/* Returns true, if plugin is correctly loaded into memory */
		inline bool loaded() {
			return mLibrary;
		}

		inline void onUnload() {
			mLibrary->onUnload();
		}
};

template <class T>
class PluginPtr {
	public:
		/*! Standard constructor 
		 * \throws std::bad_cast Throwen if instance can't be dynamicaly casted to type T
		 * \note Strong exception safety ensured.
		 */
		inline PluginPtr() : mHandle(NULL), mCreator(NULL) {}
		inline PluginPtr(pluginHandle_t *handle) throw(std::bad_cast) : mHandle(handle), mCreator((handle)?&dynamic_cast<T &>(*handle->creator()):NULL) { if(mHandle) mHandle->incRC(); }
		inline PluginPtr(const PluginPtr &other) throw() : mHandle(other.mHandle), mCreator(other.mCreator) { if(mHandle) mHandle->incRC(); }
		inline ~PluginPtr() { if(mHandle && mHandle->decRC()) mHandle->onUnload(); }
		inline T *operator->() const throw() { return mCreator; }

		inline PluginPtr &operator=(const PluginPtr &other) {
			if(mHandle == other.mHandle) return *this;
			if(mHandle) mHandle->decRC();
			mHandle = other.mHandle;
			mCreator = other.mCreator;
			if(mHandle) mHandle->incRC();
		}
		inline bool operator>(const PluginPtr &other) { return mHandle > other.mHandle; }
		inline bool operator==(const PluginPtr &other) { return mHandle == other.mHandle; }
	private:

		/*! Reference to corresponding instance of pluginHandle_t */
		pluginHandle_t *mHandle;

		/*! Creator casted to correct type to save time */
		T *mCreator;
};

class BPluginRef {
	public:
		inline BPluginRef(pluginHandle_t &handle) throw() : mHandle(&handle) { mHandle->incRC(); }
		inline BPluginRef(const BPluginRef &other) throw() : mHandle(other.mHandle) { mHandle->incRC(); }
		inline ~BPluginRef() { if(mHandle->decRC()) mHandle->onUnload(); }

		
		/*! Returns specialized instance of PluginPtr */
		template <class T>
		inline PluginPtr<T> as() const throw(std::bad_cast) { assert(mHandle); return PluginPtr<T>(mHandle); }
	private:
		/*! Assignment operator is disabled */
		inline void operator=(const BPluginRef &){}

		/*! Reference to corresponding instance of pluginHandle_t */
		pluginHandle_t *mHandle;
};

} // namespace InstantSend 

#endif
