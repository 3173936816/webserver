#ifndef __WEBSERVER_CONFIG_H__
#define __WEBSERVER_CONFIG_H__

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <typeinfo>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>

#include "log.h"

namespace webserver {

class ConfigVarBase {
public:
	using ptr = std::shared_ptr<ConfigVarBase>;

	ConfigVarBase(const std::string& name, const std::string& decription);
	virtual ~ConfigVarBase() = default;
	
	std::string getName() const { return m_name; }
	std::string getDecription() const { return m_decription; }

	virtual void serialize(const std::string& text) = 0;
	virtual std::string deserialize() const = 0;
	virtual std::string getType() const = 0;

private:
	std::string m_name;
	std::string m_decription;
};

//main template
template <class F, class T>
class LexicalCast {
public:
	T operator()(const F& f) const {
		return boost::lexical_cast<T>(f);
	}
};

//string
template <>
class LexicalCast<std::string, std::string> {
public:
	std::string operator()(const std::string& str) const {
		if(str == "") {
			return "";
		}
		YAML::Node node = YAML::Load(str);
		if(node.IsNull()) {
			node = YAML::Load('"' + str + '"');
			std::ostringstream oss;
			oss << node;
			return oss.str();
		}
		return node.Scalar();
	}
};

namespace stl_util {

	//Hash
	template <class T>
	class Hash {
	public:
		size_t operator()(const T& rhs) const {
			return std::hash<T>()(rhs);
		}
	};
	
	//Euqal_To
	template <class T>
	class Equal_To {
	public:
		bool operator()(const T& lhs, const T& rhs) const {
			return std::equal_to<T>()(lhs, rhs);
		}
	};
	
	//Compare
	template <class T>
	class Compare {
	public:
		bool operator()(const T& lhs, const T& rhs) const {
			return lhs < rhs;
		}
	};
	
}	// namespace stl_util


//vector
template <class T>
using Vector = std::vector<T>;

template <class T>
class LexicalCast<std::string, Vector<T> > {
public:
	Vector<T> operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsSequence() && !node.IsNull()) {
			throw LexicalCast_error("string to vector fmt error"); 
		}
		Vector<T> vec;
		for(size_t i = 0; i < node.size(); ++i) {
			std::ostringstream oss;
			oss << node[i];	
			vec.push_back(LexicalCast<std::string, T>()(oss.str()));
		}
		return vec;
	}
}; 

template <class T>
class LexicalCast<Vector<T>, std::string> {
public:
	std::string operator()(const Vector<T>& vec) const {
		YAML::Node node;
		for(size_t i = 0; i < vec.size(); ++i) {
			std::string str = LexicalCast<T, std::string>()(vec[i]);
			node.push_back(YAML::Load(str));
		}
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

//list
template <class T>
using List = std::list<T>;

template <class T>
class LexicalCast<std::string, List<T> > {
public:
	List<T> operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsSequence() && !node.IsNull()) {
			throw LexicalCast_error("string to list fmt error"); 
		}
		List<T> lis;
		for(size_t i = 0; i < node.size(); ++i) {
			std::ostringstream oss;
			oss << node[i];	
			lis.push_back(LexicalCast<std::string, T>()(oss.str()));
		}
		return lis;
	}
}; 

template <class T>
class LexicalCast<List<T>, std::string> {
public:
	std::string operator()(const List<T>& lis) const {
		YAML::Node node;
		for(auto& it : lis) {
			std::string str = LexicalCast<T, std::string>()(it);
			node.push_back(YAML::Load(str));
		}
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

//set
template <class T>
using Set = std::set<T, stl_util::Compare<T> >;

template <class T>
class LexicalCast<std::string, Set<T> > {
public:
	Set<T> operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsSequence() && !node.IsNull()) {
			throw LexicalCast_error("string to set fmt error"); 
		}
		Set<T> se;
		for(size_t i = 0; i < node.size(); ++i) {
			std::ostringstream oss;
			oss << node[i];	
			se.insert(LexicalCast<std::string, T>()(oss.str()));
		}
		return se;
	}
}; 

template <class T>
class LexicalCast<Set<T>, std::string> {
public:
	std::string operator()(const Set<T>& se) const {
		YAML::Node node;
		for(auto& it : se) {
			std::string str = LexicalCast<T, std::string>()(it);
			node.push_back(YAML::Load(str));
		}
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

//unordered_set
template <class T>
using Unordered_set = std::unordered_set<T, stl_util::Hash<T>, stl_util::Equal_To<T> >;

template <class T>
class LexicalCast<std::string, Unordered_set<T> > {
public:
	Unordered_set<T> operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsSequence() && !node.IsNull()) {
			throw LexicalCast_error("string to unordered_set fmt error"); 
		}
		Unordered_set<T> use;
		for(size_t i = 0; i < node.size(); ++i) {
			std::ostringstream oss;
			oss << node[i];	
			std::string str = oss.str();
			use.insert(LexicalCast<std::string, T>()(str));
		}
		return use;
	}
}; 

template <class T>
class LexicalCast<Unordered_set<T>, std::string> {
public:
	std::string operator()(const Unordered_set<T>& use) const {
		YAML::Node node;
		for(auto& it : use) {
			std::string str = LexicalCast<T, std::string>()(it);
			node.push_back(YAML::Load(str));
		}
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

//map
template <class Key, class T>
using Map = std::map<Key, T, stl_util::Compare<Key> >;

template <class Key, class T>
class LexicalCast<std::string, Map<Key, T> > {
public:
	Map<Key, T> operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsMap() && !node.IsNull()) {
			throw LexicalCast_error("string to map fmt error"); 
		}
		Map<Key, T> ma;
		for(auto it = node.begin(); it != node.end(); ++it) {
			Key key;
			T t;
			std::ostringstream oss;
			oss << it->first;
			key = LexicalCast<std::string, Key>()(oss.str());
			
			oss.str("");
			oss << it->second;
			t = LexicalCast<std::string, T>()(oss.str());
			ma.insert(std::make_pair(key, t));
		}
		return ma;
	}
}; 

template <class Key, class T>
class LexicalCast<Map<Key, T>, std::string> {
public:
	std::string operator()(const Map<Key, T>& ma) const {
		YAML::Node node;
		for(auto& it : ma) {
			std::string key = LexicalCast<Key, std::string>()(it.first);
			std::string t = LexicalCast<T, std::string>()(it.second);

			YAML::Node key_node = YAML::Load(key);
			YAML::Node t_node = YAML::Load(t);
			node[key_node] = t_node;
		}
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

//unordered_map
template <class Key, class T>
using Unordered_map = std::unordered_map<Key, T, stl_util::Hash<Key>, stl_util::Equal_To<Key> >;

template <class Key, class T>
class LexicalCast<std::string, Unordered_map<Key, T> > {
public:
	Unordered_map<Key, T> operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		if(!node.IsMap() && !node.IsNull()) {
			throw LexicalCast_error("string to unordered_map fmt error"); 
		}
		Unordered_map<Key, T> uma;
		for(auto it = node.begin(); it != node.end(); ++it) {
			Key key;
			T t;
			std::ostringstream oss;
			oss << it->first;
			key = LexicalCast<std::string, Key>()(oss.str());
			
			oss.str("");
			oss << it->second;
			t = LexicalCast<std::string, T>()(oss.str());
			uma.insert(std::make_pair(key, t));
		}
	
		return uma;
	}
}; 

template <class Key, class T>
class LexicalCast<Unordered_map<Key, T>, std::string> {
public:
	std::string operator()(const Unordered_map<Key, T>& uma) const {
		YAML::Node node;
		for(auto& it : uma) {
			std::string key = LexicalCast<Key, std::string>()(it.first);
			std::string t = LexicalCast<T, std::string>()(it.second);
			
			YAML::Node key_node = YAML::Load(key);
			YAML::Node t_node = YAML::Load(t);
			node[key_node] = t_node;
		}
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};


template <class T>
class ConfigVar : public ConfigVarBase {
public:
	using ptr = std::shared_ptr<ConfigVar>;
	using Serializer = LexicalCast<std::string, T>;
	using Deserializer = LexicalCast<T, std::string>;
	using Monitor = std::function<void(const T& oldVar, const T& newVar)>; 
	
	ConfigVar(const std::string& name, const T& value, const std::string& decription)
		: ConfigVarBase(name, decription), m_variable(value) {
	}
	~ConfigVar() = default;

	void serialize(const std::string& text) override {
		try {
			setVariable(Serializer()(text));
		} catch(std::exception& e) {
			WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT())
				<< "ConfigVar::serialize error ---- " << "exception: "
				<< e.what() << "\ntext:\n" << text;
		}
	}

	std::string deserialize() const override {
		try {
			return Deserializer()(m_variable);
		} catch(std::exception& e) {
			WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT())
				<< "ConfigVar::deserialize error ---- " << "exception: "
				<< e.what() << "\nvariable type:\n" << getType();
		}
		return "<<error -- string>>";
	}
	
	std::string getType() const override {
		return typeid(T).name();
	}
	
	void setVariable(const T& variable) {
		for(auto& it : m_monitors) {
			(it.second)(m_variable, variable);
		}

		m_variable = variable;
	}

	const T& getVariable() const {
		return m_variable;
	}
	
	// Monitor
	bool addMonitor(const std::string& sign, Monitor monitor) {
		auto it = m_monitors.find(sign);
		if(it != m_monitors.end()) {
			WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
				<< "ConfigVar::addMonitor warning --- sign " << sign
				<< " is already existed";
			return false;
		}
		m_monitors.insert(std::make_pair(sign, std::move(monitor)));
		return true;
	}

	template <typename Iterator>
	void addMonitors(Iterator beg, Iterator end) {
		for(; beg != end; ++beg) {
			addMonitor(beg->first, beg->second);
		}
	}
	
	Monitor getMonitor(const std::string& sign) const {
		auto it = m_monitors.find(sign);
		if(it == m_monitors.end()) {
			WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
				<< "ConfigVar::addMonitor warning --- sign " << sign
				<< " doesn't existed";
			return nullptr;
		}
		return it->second;
	}
	
	bool delMonitor(const std::string& sign) {
		auto it = m_monitors.find(sign);
		if(it == m_monitors.end()) {
			WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
				<< "ConfigVar::addMonitor warning --- sign " << sign
				<< " doesn't existed";
			return false;
		}
		m_monitors.erase(it);
		return true;
	}
	
	void clearMoniters() {
		m_monitors.clear();
	} 

private:
	T m_variable;
	std::unordered_map<std::string, Monitor> m_monitors;
};

class Config {
	using ConfigDatas = std::vector<std::pair<std::string, ConfigVarBase::ptr> >;
	using Iterator = std::vector<std::pair<std::string, ConfigVarBase::ptr> >::iterator;
public:
	using ptr = std::shared_ptr<Config>;

	Config() = default;
	~Config() = default;

	//add configvar
	template <typename T>
	typename ConfigVar<T>::ptr addConfigVar(const std::string& name, 
			const T& value, const std::string& decription) {

		auto it = data_find(name);
		if(it != m_configDatas.end()) {
			WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
				<< "Config::addConfigVar --- warn: "
				<< name << " already existed";
			return nullptr;
		}

		typename ConfigVar<T>::ptr newConfigVar(new ConfigVar<T>(name, value, decription));
		m_configDatas.push_back(std::make_pair(name, newConfigVar));
		return newConfigVar;
	}
	
	bool addConfigVar(ConfigVarBase::ptr configVar);
	
	template <typename T>
	typename ConfigVar<T>::ptr getConfigVar(const std::string& name) {

		auto it = data_find(name);
		if(it == m_configDatas.end()) {
			WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
				<< "Config::getConfigVar --- warn: "
				<< name << " doesn't existed";
			return nullptr;
		}
		
		typename ConfigVar<T>::ptr resConfigVar
			 = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
		if(resConfigVar == nullptr) {
			WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
				<< "Config::getConfigVar --- warn: "
				<< name << " existed, but type is "
				<< it->second->getType() << " not " << typeid(T).name();
			return nullptr;	
		}
		return resConfigVar;
	}
	
	ConfigVarBase::ptr getConfigVarBase(const std::string& name);

	bool delConfigVar(const std::string& name);
	
	static Config::ptr GetInstance();	

	//configs effect
	static void EffectConfig(const std::string& path);
	
	//yaml restore
	static void RestoreYaml(const std::string& path);

private:
	//load yaml file
	void loadFromYaml(const std::string& path);

private:
	Iterator data_find(const std::string& name);

private:
	ConfigDatas m_configDatas;
};

using ConfigSingleton = SingletonPtr<Config>;

}	// namespace webserver

#endif  // __WEBSERVER_CONFIG_H__
