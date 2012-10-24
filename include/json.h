#ifndef CONFIGT
#define CONFIGT

#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace std;

typedef int intL_t;
typedef double floatL_t;

class jsonNotExist : public exception {
	private:
		string msg;
	public:
		inline jsonNotExist(const string &name) {
			msg = string("Item \"") + name + string("\" does not exist");
		}

		virtual const char *what() const throw() {
			return msg.c_str();
		}

		~jsonNotExist() throw();
};

class jsonSyntaxErr : public exception {
	private:
		string *s;
		auto_ptr<string> ownstr;
		int p;
	public:
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
		virtual string toString() = 0;
		virtual void fromString(string *str) = 0;
		virtual jsonComponent_t *clone() const = 0;
		virtual ~jsonComponent_t();
};

class jsonInt_t: public jsonComponent_t {
	public:
		jsonInt_t(intL_t value) {
			val = value;
		}
		jsonInt_t(string *str);

		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const {
			return new jsonInt_t(val);
		}

		inline intL_t getVal() {
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

class jsonFloat_t: public jsonComponent_t {
	public:
		jsonFloat_t(floatL_t value);
		jsonFloat_t(string *str);
		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const {
			return new jsonFloat_t(val);
		}

		inline floatL_t getVal() {
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

class jsonStr_t: public jsonComponent_t {
	public:
		jsonStr_t();
		jsonStr_t(string *str);
		inline jsonStr_t(const char *str) {
			val = string(str);
		}
		string toString();
		void fromString(string *str);

		string getVal() {
			return val;
		}

		inline void setVal(string value) {
			val = value;
		}

		jsonComponent_t *clone() const {
			jsonStr_t *tmp = new jsonStr_t();
			tmp->setVal(val);
			return tmp;
		}

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

class jsonBool_t:  public jsonComponent_t {
	public:
		jsonBool_t(bool value);
		jsonBool_t(string *str);
		string toString();
		void fromString(string *str);
		jsonComponent_t *clone() const {
			return new jsonBool_t(val);
		}

		inline bool getVal() {
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

class jsonNull_t:  public jsonComponent_t {
	public:
		jsonNull_t() {}
		jsonNull_t(string *str) {
			fromString(str);
		}
		jsonComponent_t *clone() const {
			return new jsonNull_t();
		}

		string toString() {
			return "null";
		}

		void fromString(string *str) {
			if(str->substr(0, 4) == "null") str->erase(0, 4); // else throw ...
		}
};

class jsonStructuredComponent_t : public jsonComponent_t {
	virtual void deleteContent() = 0;
};

class itemContainer_t {
	private:
		bool owner;
	public:
		jsonComponent_t *item;

		inline itemContainer_t() {
			owner = false;
		}

		inline itemContainer_t(jsonComponent_t *value, bool owns) {
			item = value;
			owner = owns;
		}

		inline bool isOwner() const {
			return owner;
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

		inline jsonComponent_t *&value() {
			return mapiterator->second.item;
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

		inline jsonComponent_t *&operator[] (int ptr) {
			return data[ptr].item;
		}

		inline bool empty() {
			return data.empty();
		}

		void iterate(void (* func)(jsonComponent_t *)) {
			for(unsigned int i = 0; i < data.size(); ++i) func(data[i].item);
		}

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

		inline jsonComponent_t *&operator[] (const string &key) {
			return data[key].item;
		}

		inline jsonComponent_t *&operator[] (const char *key) {
			return data[string(key)].item;
		}

		inline bool exists(const string &key) {
			return data.count(key) > 0;
		}

		inline bool exists(const char *key) {
			return data.count(string(key)) > 0;
		}

		inline jsonComponent_t &gie(const string &key) {
			if(data.count(key) > 0 && data[key].item) return *data[key].item; else throw jsonNotExist(key);
		}

		inline jsonComponent_t &gie(const char *key) {
			if(data.count(string(key)) > 0) return *data[string(key)].item; else throw jsonNotExist(string(key));
		}

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
		inline void insertNew(const char *key, jsonComponent_t *value) {
			insertNew(string(key), value);
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

/*Obsolette
int cfgIsObj(jsonComponent_t *config);
int cfgIsArr(jsonComponent_t *config);
int cfgIsInt(jsonComponent_t *config);
int cfgIsFloat(jsonComponent_t *config);
int cfgIsNum(jsonComponent_t *config);
int cfgIsBool(jsonComponent_t *config);
int cfgIsNull(jsonComponent_t *config);
int cfgIsStr(jsonComponent_t *config);
intL_t cfgGetValInt(jsonInt_t *config);
floatL_t cfgGetValFloat(jsonComponent_t *config);	// can be eighter jsonFloat_t or jsonInt_t
int cfgGetValBool(jsonBool_t *config);
const char *cfgGetValStr(jsonStr_t *config);
jsonComponent_t *cfgGetChild(jsonObj_t *config, const char *key);
jsonComponent_t *cfgGetChildIfExists(jsonObj_t *config, const char *key);
jsonComponent_t *cfgGetItem(jsonArr_t *config, int ptr);
int cfgItemCount(jsonComponent_t *config);		// can be eighter jsonArr_t or jsonObj_t
int cfgIsEmpty(jsonComponent_t *config);		// can be eighter jsonArr_t or jsonObj_t
int cfgChildExists(jsonObj_t *config, const char *key);
*/
#endif
