#include "http.h"
#include "config.h"

#include <strings.h>
#include <algorithm>
#include <sstream>

namespace webserver {
namespace http {

namespace {

//http configuration integration
static ConfigVar<Vector<Map<std::string, std::string> > >::ptr Http_Config_Var = 
	ConfigSingleton::GetInstance()
		->addConfigVar<Vector<Map<std::string, std::string> > >("http",
			Vector<Map<std::string, std::string> >{
				{{"http_request_buff_size", "4 * 1024"}},
				{{"http_request_max_body_size", "64 * 1024 * 1024"}},
				{{"http_response_buff_size", "4 * 1024"}},
				{{"http_response_max_body_size", "64 * 1024 * 1024"}}
			}, "http specify data");

static uint64_t http_request_buff_size = 0; 
static uint64_t http_request_max_body_size = 0; 
static uint64_t http_response_buff_size = 0; 
static uint64_t http_response_max_body_size = 0; 

struct HttpConfig_Initer {
	using Type = Vector<Map<std::string, std::string> >;
	
	HttpConfig_Initer() {
	auto& variable = Http_Config_Var->getVariable();
#define XX(name) \
	if(it2.first == #name) { name = StringCalculate(it2.second); continue; }

		for(auto& it : variable) {
			for(auto& it2 : it)	{
				XX(http_request_buff_size)
				XX(http_request_max_body_size)
				XX(http_response_buff_size) 
				XX(http_response_max_body_size)
			}
		}
		Http_Config_Var->addMonitor("http_01", [](const Type& oldVar, const Type& newVar) {
			for(auto& it : newVar) {
				for(auto& it2 : it)	{
					XX(http_request_buff_size)
					XX(http_request_max_body_size)
					XX(http_response_buff_size) 
					XX(http_response_max_body_size)
				}
			}
		});
	}
#undef XX
};

static HttpConfig_Initer HttpConfig_Initer_CXX_01;
}

uint64_t GetHttpRequestBuffSize() {
	return http_request_buff_size;
}

uint64_t GetHttpRequestMaxBodySize() {
	return http_request_max_body_size;
}

uint64_t GetHttpResponseBuffSize() {
	return http_response_buff_size;
}

uint64_t GetHttpResponseMaxBodySize() {
	return http_response_max_body_size;
}

std::string HttpStatusToString(HttpStatus status) {
#define XX(num, name, decription)	\
	if(num == static_cast<int>(status)) {	\
		return #name;	\
	}
	HTTP_STATUS_MAP(XX)
#undef XX
	return "INVALID";
}

HttpStatus StringToHttpStatus(const std::string& status_str) {
#define XX(num, name, decription)	\
	if(strcasecmp(#name, status_str.c_str()) == 0) {	\
		return HttpStatus::name;	\
	}
	HTTP_STATUS_MAP(XX)
#undef XX
	return HttpStatus::INVALID;
}

std::string GetHttpStatusDescription(HttpStatus status) {
#define XX(num, name, decription)	\
	if(num == static_cast<int>(status)) {	\
		return #decription;	\
	}
	HTTP_STATUS_MAP(XX)
#undef XX
	return "INVALID";
}

std::string HttpMethodToString(HttpMethod method) {
#define XX(num, name, str) \
	if(num == static_cast<int>(method)) {	\
		return #name;	\
	}
	HTTP_METHOD_MAP(XX)
#undef XX
	return "INVALID";
}

HttpMethod StringToHttpMethod(const std::string& method_str) {
#define XX(num, name, str)	\
	if(strcasecmp(#name, method_str.c_str()) == 0) {	\
		return HttpMethod::name;	\
	}
	HTTP_METHOD_MAP(XX)
#undef XX
	return HttpMethod::INVALID;
}

// HttpRequest

HttpRequest::HttpRequest(HttpMethod method, uint8_t version)
	: m_method(method), m_path("/"), m_query{}
	, m_fragment{}, m_version(version)
	, m_headers{}, m_body{} {
}

HttpRequest::HttpRequest()
	: m_method(HttpMethod::INVALID)
	, m_path{}, m_query{}
	, m_fragment{}, m_version(0)
	, m_headers{}, m_body{} {
}

HttpRequest::~HttpRequest() = default;

void HttpRequest::addHeaderField(const std::string& key, const std::string& value) {
	m_headers.emplace_back(key, value);
}

std::string HttpRequest::findHeaderField(const std::string& key, const std::string& def) {
	for(auto& it : m_headers) {
		if(it.first == key) {
			return it.second;
		}
	}
	return def;
}

bool HttpRequest::delHeaderField(const std::string& key) {
	auto it = std::find_if(m_headers.begin(), m_headers.end()
			, [key](const std::pair<std::string, std::string>& elem_pair){
		return elem_pair.first == key;	
	});
	if(it == m_headers.end()) {
		return false;
	}
	m_headers.erase(it);
	return true;
}

std::string HttpRequest::getRequest() const {
	std::ostringstream oss;
	oss << HttpMethodToString(m_method) << " " << m_path;
	if(!m_query.empty()) {
		oss << "?" << m_query;
	}
	if(!m_fragment.empty()) {
		oss << "#" << m_fragment;
	}
	
	oss << " HTTP/" << (m_version >> 4) 
		<< "." << (m_version & 0x0f) << "\r\n";
	
	for(auto& it : m_headers) {
		oss << it.first << ": " << it.second << "\r\n";
	}
	oss << "\r\n";
	oss << m_body;
	
	return oss.str();
}

HttpResponse::HttpResponse(HttpStatus status, uint8_t version)
	: m_version(version), m_status(status)
	, m_headers{}, m_body{} {
}

HttpResponse::HttpResponse()
	: m_version(0)
	, m_status(HttpStatus::INVALID)
	, m_headers{}, m_body{} {
}

HttpResponse::~HttpResponse() = default;

void HttpResponse::addHeaderField(const std::string& key, const std::string& value) {
	m_headers.emplace_back(key, value);
}

std::string HttpResponse::findHeaderField(const std::string& key, const std::string& def) {
	for(auto& it : m_headers) {
		if(it.first == key) {
			return it.second;
		}
	}
	return def;
}

bool HttpResponse::delHeaderField(const std::string& key) {
	auto it = std::find_if(m_headers.begin(), m_headers.end()
			, [key](const std::pair<std::string, std::string>& elem_pair){
		return elem_pair.first == key;	
	});
	if(it == m_headers.end()) {
		return false;
	}
	m_headers.erase(it);
	return true;
}

std::string HttpResponse::getResponse() const {
	std::ostringstream oss;
	oss << "HTTP/" << (m_version >> 4) 
		<< "." << (m_version & 0x0f) << " "
		<< static_cast<int>(m_status) << " "
		<< GetHttpStatusDescription(m_status) << "\r\n";
	for(auto& it : m_headers) {
		oss << it.first << ": " << it.second << "\r\n";
	} 
	oss << "\r\n";
	oss << m_body;
	
	return oss.str();
}

}	// namespace http
}	// namespace webserver
