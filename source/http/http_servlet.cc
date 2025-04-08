#include "http_servlet.h"
#include <fnmatch.h>

namespace webserver {
namespace http {

// Servlet
Servlet::Servlet() = default;

Servlet::~Servlet() = default;

// FunctionServlet
FunctionServlet::FunctionServlet(Func func)
	: Servlet(), m_func(func) {
}

FunctionServlet::~FunctionServlet() = default;

void FunctionServlet::handle(HttpSession::ptr session
		, HttpRequest::ptr request, HttpResponse::ptr response) {

	m_func(session, request, response);
}

ServletDispatch::ServletDispatch()
	: Servlet(), m_exactServlets{}
	, m_globServlets{}, m_defaultServlet{} {
}

ServletDispatch::~ServletDispatch() = default;

void ServletDispatch::handle(HttpSession::ptr session
		, HttpRequest::ptr request, HttpResponse::ptr response) {

	Servlet::ptr servlet = uriMatchServlet(request->getPath());
	if(servlet) {
		servlet->handle(session, request, response);
	}
}

bool ServletDispatch::addExactServlet(const std::string& uri, Servlet::ptr servlet) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_exactServlets.find(uri);
	if(it != m_exactServlets.end()) {
		return false;
	}
	m_exactServlets[uri] = servlet;
	return true;
}

bool ServletDispatch::addExactServlet(const std::string& uri, FunctionServlet::Func func) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_exactServlets.find(uri);
	if(it != m_exactServlets.end()) {
		return false;
	}
	FunctionServlet::ptr servlet = std::make_shared<FunctionServlet>(func);
	m_exactServlets[uri] = servlet;
	return true;
}

bool ServletDispatch::setExactServlet(const std::string& uri, Servlet::ptr servlet) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_exactServlets.find(uri);
	if(it == m_exactServlets.end()) {
		return false;
	}
	m_exactServlets[uri] = servlet;
	return true;
}

bool ServletDispatch::setExactServlet(const std::string& uri, FunctionServlet::Func func) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_exactServlets.find(uri);
	if(it == m_exactServlets.end()) {
		return false;
	}
	FunctionServlet::ptr servlet = std::make_shared<FunctionServlet>(func);
	m_exactServlets[uri] = servlet;
	return true;
}

bool ServletDispatch::delExactServlet(const std::string& uri) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_exactServlets.find(uri);
	if(it == m_exactServlets.end()) {
		return false;
	}
	m_exactServlets.erase(it);
	return true;
}

Servlet::ptr ServletDispatch::findExactServlet(const std::string& uri) {
	MutexType::RDLock locker(m_rwmtx);
	auto it = m_exactServlets.find(uri);
	if(it == m_exactServlets.end()) {
		return nullptr;
	}
	return it->second;
}

bool ServletDispatch::addGlobServlet(const std::string& uri, Servlet::ptr servlet) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_globServlets.find(uri);
	if(it != m_globServlets.end()) {
		return false;
	}
	m_globServlets[uri] = servlet;
	return true;
}

bool ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::Func func) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_globServlets.find(uri);
	if(it != m_globServlets.end()) {
		return false;
	}
	FunctionServlet::ptr servlet = std::make_shared<FunctionServlet>(func);
	m_globServlets[uri] = servlet;

	return true;
}

bool ServletDispatch::setGlobServlet(const std::string& uri, Servlet::ptr servlet) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_globServlets.find(uri);
	if(it == m_globServlets.end()) {
		return false;
	}
	m_globServlets[uri] = servlet;
	return true;
}

bool ServletDispatch::setGlobServlet(const std::string& uri, FunctionServlet::Func func) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_globServlets.find(uri);
	if(it == m_globServlets.end()) {
		return false;
	}
	FunctionServlet::ptr servlet = std::make_shared<FunctionServlet>(func);
	m_globServlets[uri] = servlet;
	return true;
}

bool ServletDispatch::delGlobServlet(const std::string& uri) {
	MutexType::WRLock locker(m_rwmtx);
	auto it = m_globServlets.find(uri);
	if(it == m_globServlets.end()) {
		return false;
	}
	m_globServlets.erase(it);
	return true;
}

Servlet::ptr ServletDispatch::findGlobServletServlet(const std::string& uri) {
	MutexType::RDLock locker(m_rwmtx);
	auto it = m_globServlets.find(uri);
	if(it == m_globServlets.end()) {
		return nullptr;
	}
	return it->second;
}

Servlet::ptr ServletDispatch::uriMatchServlet(const std::string& uri) {
	MutexType::RDLock locker(m_rwmtx);
	
	auto it = m_exactServlets.find(uri);
	if(it != m_exactServlets.end()) {
		return it->second;
	}

	for(auto& it : m_globServlets) {
		if(fnmatch(it.first.c_str(), uri.c_str(), 0) == 0) {
			return it.second;
		}
	}
	return m_defaultServlet;
}

void ServletDispatch::setDefaultServlet(Servlet::ptr servlet) {
	MutexType::WRLock locker(m_rwmtx);
	m_defaultServlet = servlet;
}

Servlet::ptr ServletDispatch::getDefaultServlet() {
	MutexType::RDLock locker(m_rwmtx);
	return m_defaultServlet;
}

// NotFoundServlet
NotFoundServlet::NotFoundServlet(const std::string& server)
	: Servlet(), m_server(server) {
}

NotFoundServlet::~NotFoundServlet() = default;

void NotFoundServlet::handle(HttpSession::ptr session
		, HttpRequest::ptr request, HttpResponse::ptr response) {
	static std::string content = 
		R"(<!DOCTYPE html>
		<html lang="en">
		<head>
		    <meta charset="UTF-8">
		    <meta name="viewport" content="width=device-width, initial-scale=1.0">
		    <title>404</title>
		</head>
		<body>
		    
		    <div align = "center">
		        <h1>404 not found</h1>
		    </div>
		    <hr size = 3 />
		
		    <div align = "center">
		        <font> )" + m_server + R"( </font>
		    </div>
		
		</body>
		</html>)";

	response->setVersion(request->getVersion());
	response->setStatus(HttpStatus::NOT_FOUND);
	response->addHeaderField("Content-Type", "text/html");
	response->addHeaderField("Content-Length", std::to_string(content.size()));
	response->setBody(content);
}

}	// nmaespace http
}	// namespace webserver
