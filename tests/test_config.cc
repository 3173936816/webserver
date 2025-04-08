#include "webserver.h"

using namespace webserver;

void test_base() {
	ConfigVar<int>::ptr var = 
		ConfigSingleton::GetInstance()->addConfigVar<int>("int", 8, "");
	ConfigVar<double>::ptr var1 = 
		ConfigSingleton::GetInstance()->getConfigVar<double>("int");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();
	var->serialize("9999");
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();
}

void test_parse() {
	Config::ptr configMgr = ConfigSingleton::GetInstance();
	
	ConfigVar<std::string>::ptr var = 
		configMgr->addConfigVar<std::string>("system", "def", "");
	Config::EffectConfig("../tests/test_yaml.yaml");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();
}

class Person {
public:
	Person() = default;
	Person(const std::string& name, uint32_t age)
		: m_name(name), m_age(age) {
	}
	~Person() = default;
	
	//bool operator<(const Person& rhs) const {
	//	return rhs.m_age < m_age;
	//}
	
	//bool operator==(const Person& rhs) const {
	//	return rhs.m_name == m_name
	//		&& rhs.m_age == m_age;
	//}

	std::string m_name;
	uint32_t m_age;
};

namespace webserver {

template <>
class LexicalCast<std::string, Person> {
public:
	Person operator()(const std::string& text) const {
		YAML::Node node = YAML::Load(text);
		Person p;
		p.m_name = node["name"].Scalar();
		p.m_age = LexicalCast<std::string, uint32_t>()(node["age"].Scalar());
		return p;
	}
};

template <>
class LexicalCast<Person, std::string> {
public:
	std::string operator()(const Person& p) const {
		YAML::Node node;
		node["name"] = p.m_name;
		node["age"] = p.m_age;
		
		std::ostringstream oss;
		oss << node;
		return oss.str();
	}
};

template <>
class stl_util::Compare<Person> {
public:
	bool operator()(const Person& lhs, const Person& rhs) const {
		return lhs.m_age < rhs.m_age;
	}
};

template <>
class stl_util::Equal_To<Person> {
public:
	bool operator()(const Person& lhs, const Person& rhs) const {
		return lhs.m_age == rhs.m_age
			&& lhs.m_name == rhs.m_name;
	}
};

template <>
class stl_util::Hash<Person> {
public:
	size_t operator()(const Person& rhs) const {
		return std::hash<std::string>()(rhs.m_name);
	}
};

}

void test_person() {
	ConfigVar<Person>::ptr var = ConfigSingleton::GetInstance()
		->addConfigVar<Person>("person", Person("lcy", 22), "");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();

	Config::EffectConfig("../tests/test_yaml.yaml");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();
}

//template <>
//class std::hash<Person> {
//public:
//	size_t operator()(const Person& rhs) const {
//		return std::hash<std::string>()(rhs.m_name);
//	}
//};

using Type = Unordered_set<std::string>;

void test_person2() {
	ConfigVar<Type>::ptr var = ConfigSingleton::GetInstance()
		->addConfigVar<Type>(
		"person", Type{}/*Type{Person("lcy", 22)}*/, "");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();

	Config::EffectConfig("../tests/test_yaml.yaml");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();
	const Type& vec = var->getVariable();
	for(auto& it : vec) {
		//std::clog << it.m_name << "  " << it.m_age << std::endl;
		std::cout << it << std::endl;
	}
}

using MapType = Map<std::string, Vector<Person> >;

void test_person3() {
	ConfigVar<MapType>::ptr var = ConfigSingleton::GetInstance()
		->addConfigVar<MapType>(
		"person_map", MapType{}, "");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();

	Config::EffectConfig("../tests/test_yaml.yaml");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << var->deserialize();
	const MapType& vec = var->getVariable();
	for(auto& it : vec) {
		std::cout << it.first << " : ";
		
		for(auto& e : it.second) {
			std::cout << e.m_name << "  "
			<< e.m_age << std::endl;
		}
	}
}

void test_monitor() {
	ConfigVar<MapType>::ptr var = ConfigSingleton::GetInstance()
		->addConfigVar<MapType>(
		"person_map", MapType{}, "");
	
	std::vector<std::pair<std::string, ConfigVar<MapType>::Monitor> > vec;
	vec.push_back(std::make_pair("test_monitor", [](const MapType& oldVar, 
		const MapType& newVar) {
		std::cout << "test monitor successful" << std::endl;
	}));
	
	var->addMonitors(vec.begin(), vec.end());
	ConfigVar<MapType>::Monitor m = var->getMonitor("test_monitor");
	m(MapType{}, MapType{});

	Config::EffectConfig("../tests/test_yaml.yaml");
}

void test_logyaml() {
	auto var = ConfigSingleton::GetInstance()->getConfigVarBase("logs");

	ConfigVar<Person>::ptr var1 = ConfigSingleton::GetInstance()
		->addConfigVar<Person>(
		"person", Person("lcy", 22), "");
	
	Config::EffectConfig("../webserver_confs.yml");
	
	std::cout << var->deserialize() << std::endl;
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << "++++++++++++++++++++++";
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_GET("system")) << "++++++++++++++++++++++";
	
	sleep(15);
	Config::RestoreYaml("../webserver_confs_restore.yml");
}

int main() {
	//test_base();
	test_parse();	
	//test_person();
	//test_person2();
	//test_person3();
	//test_monitor();
	//test_logyaml();
	
	return 0;
}
