#ifndef PLUGIN_H
#define PLUGIN_H
#include <set>
#include <string>
#include <stdexcept>
#include <assert.h>

#include "pluginapi.h"

using namespace std;

template <class T>
struct pluginInstanceAutoPtrRef {
	T *mInstance;
	pluginInstanceAutoPtrRef(T *instance) : mInstance(instance) {}
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
		inline pluginInstanceAutoPtr &operator=(auto_ptr<T> &other) throw() { reset(other.release()); return *this; }
		inline pluginInstanceAutoPtr &operator=(const pluginInstanceAutoPtrRef<T> &other) throw() { reset(other.mInstance); return *this; }
		template<class T2>
			pluginInstanceAutoPtr &operator=(pluginInstanceAutoPtr<T2> &other) { reset(other.release()); return *this; }
		inline T &operator*() const throw() { return *mInstance; }
		inline T *operator->() const throw() { return mInstance; }
		inline operator pluginInstanceAutoPtrRef<T>() { return pluginInstanceAutoPtrRef<T>(this->release()); }
		template <class T2>
			inline operator pluginInstanceAutoPtrRef<T2>() { return pluginInstanceAutoPtrRef<T2>(this->release()); }
		void reset(T *instance = NULL) { if(mInstance) { pluginMultiInstanceCreator_t &creator(mInstance->getCreator()); delete mInstance; creator.checkUnload(); } mInstance = instance; }
		inline bool valid() { return mInstance; }
	private:
		inline T *release() {
			T *tmp(mInstance);
			mInstance = NULL;
			return tmp;
		}
		T *mInstance;
};


class EPluginInvalid : public runtime_error {
	inline EPluginInvalid(const string &message) : runtime_error(message) {};
};

class LibraryHandle {
	public:
		virtual void onUnload() = 0;
		virtual ~LibraryHandle();
		inline pluginInstanceCreator_t *creator() { return mCreator; }
	protected:
		pluginInstanceCreator_t *mCreator;
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
		pluginHandle_t(auto_ptr<LibraryHandle> library);
		pluginHandle_t(const pluginHandle_t &other);

		/* Frees associated resources and unloads library */
		~pluginHandle_t();

		inline void assignLibrary(auto_ptr<LibraryHandle> library) {
			assert(library.get());
			mLibrary = library.release();
			mOwner = true;
		}

		/*! Increases reference count */
		inline void incRC() throw() {
			++refCnt;
		}

		/*! Decreases reference count and returns true, if ti's possible to unload plugin */
		inline bool decRC() throw() {
			return !--refCnt && mLibrary->creator()->unloadPossible();
		}

		/*! Returns actual reference count */
		inline unsigned int getRC() {
			return refCnt;
		}

		inline bool isUnloadable() {
			return !refCnt && mLibrary->creator()->unloadPossible();
		}

		/* Returns pointer to instance creator */
		inline pluginInstanceCreator_t *creator() {
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
		inline PluginPtr(pluginHandle_t *handle) throw(bad_cast) : mHandle(handle), mCreator((handle)?&dynamic_cast<T &>(*handle->creator()):NULL) { if(mHandle) mHandle->incRC(); }
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
		inline PluginPtr<T> as() const throw(bad_cast) { assert(mHandle); return PluginPtr<T>(mHandle); }
	private:
		/*! Assignment operator is disabled */
		inline void operator=(const BPluginRef &){}

		/*! Reference to corresponding instance of pluginHandle_t */
		pluginHandle_t *mHandle;
};

#if 0
class plugin_t {
	private:
		pluginHandle_t *handle;
	public:
		plugin_t(const pluginHandle_t *handle);
		plugin_t(const plugin_t &plugin);
		inline plugin_t() {
			handle = NULL;
		}
		~plugin_t();
		plugin_t &operator=(plugin_t &plugin);
		pluginHandle_t *operator=(pluginHandle_t *handle);

		inline pluginInstanceCreator_t *creator() {
			return handle->creator();
		}

		inline bool loaded() {
			return handle && handle->loaded();
		}

		inline pluginInstanceAutoPtr<peer_t> newClient(const jsonComponent_t &config) {
			return dynamic_cast<connectionCreator_t &>(*handle->creator()).newClient(config);
		}

		inline pluginInstanceAutoPtr<serverPlugin_t> newServer(const jsonComponent_t &config) {
			return dynamic_cast<connectionCreator_t &>(*handle->creator()).newServer(config);
		}

		inline const char *lastError() {
			return handle->creator()->getErr();
		}

		/* \brief Returns true, if instance of plugin_t is last thing that depends on plugin
		 */
		inline bool isLast() {
			return handle->getRC() == 1 && handle->creator()->unloadPossible();
		}
};
#endif // if 0

#endif
