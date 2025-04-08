#include "config.h"

#include <yaml-cpp/yaml.h>

namespace webserver {

//ConfigVarBase
ConfigVarBase::ConfigVarBase(const std::string& name, const std::string& decription)
	: m_name(name), m_decription(decription){
}

//Config

Config::Iterator Config::data_find(const std::string& name) {
	auto it = m_configDatas.begin();
	for(; it != m_configDatas.end(); ++it) {
		if(it->first == name) {
			break;
		}
	}		

	return it;
}

bool Config::addConfigVar(ConfigVarBase::ptr configVar) {
	std::string name = configVar->getName();

	auto it = data_find(name);
	if(it != m_configDatas.end()) {
		WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
			<< "Config::addConfigVar --- warn: "
			<< name << " already existed";
		return false;
	}
	m_configDatas.push_back(std::make_pair(name, configVar));
	return true;
}

ConfigVarBase::ptr Config::getConfigVarBase(const std::string& name) {
	auto it = data_find(name);
	if(it == m_configDatas.end()) {
		return nullptr;
	}
	return it->second;
}

bool Config::delConfigVar(const std::string& name) {
	auto it = data_find(name);
	if(it == m_configDatas.end()) {
		WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_ROOT())
			<< "Config::addConfigVar --- warn: "
			<< name << " doesn't existed";
		return false;
	}
	m_configDatas.erase(it);
	return true;
}

Config::ptr Config::GetInstance() {
	static Config::ptr instance(new Config());
	return instance;
}

static void parseYaml(YAML::Node& node, std::vector<std::pair<std::string, YAML::Node> >& vec) {
	if(!node.IsSequence()) {
		throw Config_error("yaml file format error1");
	}
	for(size_t i = 0; i < node.size(); ++i) {
		if(!node[i].IsMap() || node[i].size() > 1) {
			throw Config_error("yaml file format error2");
		}
		auto it = node[i].begin();
		std::string key = it->first.Scalar();
		vec.push_back(std::make_pair(key, it->second));
	}
} 

void Config::loadFromYaml(const std::string& path) {
	YAML::Node node;
	try {
		node = YAML::LoadFile(path);
	} catch(std::exception& e) {
		WEBSERVER_LOG_FATAL(WEBSERVER_LOGMGR_ROOT())
			<< "Config::LoadFromYaml error -- " << path
			<< " : " << e.what();
		std::abort();
	}

	std::vector<std::pair<std::string, YAML::Node> > vec;
	parseYaml(node, vec);
	
//	for(auto& it : vec) {
//		std::clog << "first : " << it.first << std::endl;
//		std::ostringstream oss;
//		oss << it.second;
//		std::clog << "second :\n " << oss.str() << std::endl;
//	}

	for(auto& it : vec) {
		ConfigVarBase::ptr varBase = getConfigVarBase(it.first);
		if(varBase) {
			std::ostringstream oss;
			oss << it.second;
			varBase->serialize(oss.str());
		}
	}
}

void Config::EffectConfig(const std::string& path) {
	ConfigSingleton::GetInstance()->loadFromYaml(path);
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) 
		<< "\n-----------Configuration loading completed------------";
}

void Config::RestoreYaml(const std::string& path) {
	std::ofstream ofs;
	ofs.open(path, std::ios_base::app);
	if(!ofs.is_open()) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT()) 
			<< "Config::RestoreYaml error";
		return;
	}

	YAML::Node node;
	for(auto& it : ConfigSingleton::GetInstance()->m_configDatas) {
		YAML::Node node1;
		node1[YAML::Load(it.first)] = YAML::Load(it.second->deserialize());
		node.push_back(node1);
	}
	std::ostringstream oss;
	oss << node;
	ofs << oss.str();
	ofs.close();
}

}	// namespace webserver
