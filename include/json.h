#ifndef CONFIGT
#define CONFIGT

#include <string>
#include <vector>
#include <map>
#include <memory>

// For debugging
#define NOPURE

#ifdef NOPURE
#define PURE
#else
#define PURE = 0
#endif

using namespace std;

typedef int intL_t;
typedef double floatL_t;

/*! Exception thrown on attempt to access not existing item in json object */
class jsonNotExist : public exception {
	private:
		/*! Human readable error message */
		string msg;
	public:
		/*! Constructor
		 * \param name Name of not existing item
		 */
		inline jsonNotExist(const string &name) {
			msg = string("Item \"") + name + string("\" does not exist");
		}

		/* Description of exception
		 * \return Null terminated human readable string */
		virtual const char *what() const throw() {
			return msg.c_str();
		}

		/* Standard (empty) destructor */
		~jsonNotExist() throw();
};

/*! Exception thrown when parsing invalid json string */
class jsonSyntaxErr : public exception {
	private:
		/*! Pointer to description of error */
		string *s;
		/*! Auto destructing container for self owned string */
		auto_ptr<string> ownstr;
		/*! Possition, where error occured */
		int p;
	public:
		/*! Creates exception with error message and position */
		inline jsonSyntaxErr(string *str, int pos) {
			s = str;
			p = pos;
		}

		inline jsonSyntaxErr(const string &str) {
			s = (ownstr = auto_ptr<string>(new string(str))).get();
			p = 0;
		}

		inline jsonSyntaxErr(const jsonSyntaxErr &err) {
			if(err.ownstr.get()) ownstr = auto_ptr<string>(new string(*err.ownstr));
		}

		virtual const char *what() const throw() {
			return s->c_str();
		}

		inline int where() {
			return p;
		}

		~jsonSyntaxErr() throw();
};

class fileNotAccesible : public exception {
	private:
		string msg;
	public:
		inline fileNotAccesible(const string &filename) {
			msg = string("Can't access file ") + filename;
		}

		const char *what() {
			return msg.c_str();
		}

		~fileNotAccesible() throw();
};

class fileUnreadable : public exception {
	private:
		string msg;
	public:
		inline fileUnreadable(const string &filename) {
			msg = string("Can't access file ") + filename;
		}

		virtual const char *what() throw() {
			return msg.c_str();
		}

		~fileUnreadable() throw();
};

// Json class (compozite design pattern)
class jsonComponent_t
{
	public:
		virtual string toString() PURE;
		virtual void fromString(string *str) PURE;
		virtual jsonComponent_t *clone() const PURE;
		virtual ~jsonComponent_t();
};

class jsonInt_t : public jsonComponent_t {
	public:
		jsonInt_t(intL_t value) {
			val = value;
		}
		jsonInt_t(string *str);
		~jsonInt_t();

		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const;
		inline intL_t getVal() const {
			return val;
		}

		inline void setVal(intL_t value) {
			val = value;
		}
		inline intL_t operator+(jsonInt_t &a) {
			return getVal() + a.getVal();
		}
		/*jsonFloat_t &operator+(jsonFloat_t &a) {
			return jsonFloat_t(getVal + a.getVal);
		}*/

		inline intL_t operator-(jsonInt_t &a) {
			return getVal() - a.getVal();
		}

		/*floatL_t &operator-(jsonFloat_t &a) {
			return jsonFloat_t(getVal - a.getVal);
		}*/
		inline bool operator<(int a) {
			return getVal() < a;
		}

		inline bool operator>(int a) {
			return getVal() > a;
		}

		inline intL_t operator=(const int value) {
			return val = value;
		}

		/*inline jsonInt_t &operator=(jsonInt_t &value) {
			val = value.val;
		}*/
	private:
		intL_t val;
};

class jsonFloat_t : public jsonComponent_t {
	public:
		jsonFloat_t(floatL_t value);
		jsonFloat_t(string *str);
		~jsonFloat_t();
		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const;

		inline floatL_t getVal() const {
			return val;
		}
		inline void setVal(floatL_t value) {
			val = value;
		}

		floatL_t operator+(jsonInt_t &a) {
			return getVal() + a.getVal();
		}

		floatL_t operator-(jsonInt_t &a) {
			return getVal() - a.getVal();
		}

		inline floatL_t operator=(const int value) { 
			return val = value;
		}                                              

	private:
		floatL_t val;
};

class jsonStr_t : public jsonComponent_t {
	public:
		jsonStr_t();
		jsonStr_t(string *str);
		~jsonStr_t();
		inline jsonStr_t(const char *str) {
			val = string(str);
		}
		string toString();
		void fromString(string *str);

		string getVal() const {
			return val;
		}

		inline void setVal(string value) {
			val = value;
		}

		jsonComponent_t *clone() const;

		inline bool operator==(const char *str) {
			return val == string(str);
		}

		inline bool operator==(const string &str) {
			return val == str;
		}

		inline bool operator!=(const char *str) {
			return val != string(str);
		}

		inline bool operator!=(const string &str) {
			return val != str;
		}

		inline const char *operator=(const char *value) {
			val = string(value);
			return value;
		}                                             

		inline const string &operator=(const string value) {
			return val = value;                                  
		}                                             

	private:
		string val;
};

class jsonBool_t : public jsonComponent_t {
	public:
		jsonBool_t(bool value);
		jsonBool_t(string *str);
		~jsonBool_t();
		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const;

		inline bool getVal() const {
			return val;
		}

		inline void setVal(bool value) {
			val = value;
		}

		inline bool operator=(const bool value) {
			return val = value;
		}                                             

	private:
		bool val;
};

class jsonNull_t :  public jsonComponent_t {
	public:
		jsonNull_t() {}
		jsonNull_t(string *str) {
			fromString(str);
		}
		~jsonNull_t();
		jsonComponent_t *clone() const;

		string toString() {
			return "null";
		}

		void fromString(string *str) {
			if(str->substr(0, 4) == "null") str->erase(0, 4); // else throw ...
		}
};

/*
class jsonStructuredComponent_t : public jsonComponent_t {
	public:
		virtual void deleteContent() = 0;
		~jsonStructuredComponent_t();
};
*/

typedef jsonComponent_t jsonStructuredComponent_t;

class itemContainer_t {
	private:
		unsigned int *mRC;
		jsonComponent_t *mItem;

		inline void invalidate() {
			if(mRC && !--*mRC) {
				delete mRC;
				delete mItem;
				mRC = NULL;
			}
		}
	public:

		inline itemContainer_t() : mRC(NULL), mItem(NULL) {
			//puts("itemContainer_t created");
		}
		inline itemContainer_t(const itemContainer_t &other) : mRC(other.mRC), mItem(other.mItem) { if(mRC) ++*mRC; }

		inline itemContainer_t(jsonComponent_t *value, bool owns) : mRC(owns?new unsigned int:NULL), mItem(value) {
			if(owns) *mRC = 1;
		}

		inline bool isOwner() const {
			return mRC;
		}

		inline itemContainer_t &operator=(const itemContainer_t &other) {
			invalidate();
			mRC = other.mRC;
			mItem = other.mItem;
			return *this;
		}

		inline jsonComponent_t &operator*() const {
			if(!mItem) throw jsonNotExist("UNKNOWN");
			return *mItem;
		}

		inline jsonComponent_t *operator->() const {
			return mItem;
		}

		inline ~itemContainer_t() {
			invalidate();
		}
};

class jsonIterator {
	private:
		map<string, itemContainer_t>::iterator mapiterator;
	public:
		inline jsonIterator(map<string, itemContainer_t>::iterator mi) {
			mapiterator = mi;
		}

		inline jsonIterator& operator++() {
			++mapiterator;
			return *this;
		}

		inline jsonIterator operator++(int) {
			jsonIterator tmp = jsonIterator(mapiterator);
			++mapiterator;
			return tmp;
		}

		inline string key() {
			return mapiterator->first;
		}

		inline jsonComponent_t &value() {
			return *mapiterator->second;
		}

		inline bool operator!=(const jsonIterator &it) {
			return mapiterator != it.mapiterator;
		}
};

class jsonArr_t: public jsonStructuredComponent_t {
	public:
		inline jsonArr_t() {;}
		inline jsonArr_t(string *str) {
			fromString(str);
		}
		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const;

		jsonArr_t &operator=(const jsonArr_t &other);
		inline jsonArr_t(const jsonArr_t &other) {
			*this = other;
		}

		inline void addVal(jsonComponent_t *value) {
			data.push_back(itemContainer_t(value, false));
		}

		inline void addNew(jsonComponent_t *value) {
			data.push_back(itemContainer_t(value, true));
		}

		inline jsonComponent_t &operator[] (int ptr) {
			return *data[ptr];
		}

		inline bool empty() {
			return data.empty();
		}

		/*void iterate(void (* func)(jsonComponent_t *)) {
			for(unsigned int i = 0; i < data.size(); ++i) func(data[i].item);
		}*/

		inline int count() {
			return data.size();
		}

		void deleteContent();
		~jsonArr_t();

	private:
		vector<itemContainer_t> data;
};

class jsonObj_t: public jsonStructuredComponent_t {
	public:
		inline jsonObj_t() {};
		inline jsonObj_t(string *str) {
			fromString(str);
		}

		jsonObj_t &operator=(const jsonObj_t &other);

		inline jsonObj_t(const jsonObj_t &other) {
			*this = other;
		}

		inline jsonObj_t(const char *str) {
			string tmp = string(str);
			fromString(&tmp);
		}

		inline const jsonComponent_t &operator[] (const string &key) const {
			return gie(key);
		}

		inline jsonComponent_t &operator[] (const string &key) {
			return gie(key);
		}

		inline bool exists(const string &key) {
			return data.count(key) > 0;
		}

		inline jsonComponent_t &gie(const string &key) const {
			map<string, itemContainer_t>::const_iterator it(data.find(key));
			if(it == data.end()) throw jsonNotExist(key);
			return *it->second;
		}
/*
		inline jsonComponent_t &gie(const string &key) {
			map<string, itemContainer_t>::iterator it(data.find(key));
			if(it == data.end()) throw jsonNotExist(key);
			return *it->second;
		}
*/
		inline bool empty() {
			return data.empty();
		}

		inline jsonIterator begin() {
			return jsonIterator(data.begin());
		}

		inline jsonIterator end() {
			return jsonIterator(data.end());
		}

		void insertNew(const string &key, jsonComponent_t *value);
		inline void insertVal(const string &key, jsonComponent_t *value) {
			data[key] = itemContainer_t(value, false);
		}

		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const;

		inline int count() {
			return data.size();
		}

		void deleteContent();
		~jsonObj_t();
	private:
		int type;
		map<string, itemContainer_t> data;
};

/* // Idea for universal access to Json components
class JALParser_t {
	protected:
		auto_ptr<JALParser_t> subparser;
		inline JALParser_t() {;}
	public:
		JALParser_t(const string &str);
		virtual jsonComponent_t *&access(jsonComponent_t &json);
};

class JALObjParser_t : public JALParser_t {
	private:
		string key;
	public:
		JALObjParser_t(const string &str);
		jsonComponent_t *&access(jsonComponent_t &json);
};

class JALArrParser_t : public JALParser_t {
	private:
		int index;
	public:
		JALArrParser_t(const string &str);
		jsonComponent_t *&access(jsonComponent_t &json);
};
*/

// Wrapper functions
auto_ptr<jsonComponent_t> cfgReadStr(const char *str);
auto_ptr<jsonComponent_t> cfgReadFile(const char *path);

#endif
