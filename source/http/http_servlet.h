#ifndef __WEBSERVER_HTTP_SERVLET_H__
#define __WEBSERVER_HTTP_SERVLET_H__

#include <memory>
#include <functional>

#include "http.h"
#include "http_session.h"
#include "../synchronism.h"

namespace webserver {
namespace http {

class Servlet {
public:
	using ptr = std::shared_ptr<Servlet>;
	
	Servlet();
	virtual ~Servlet();

	virtual void handle(HttpSession::ptr session
			, HttpRequest::ptr request, HttpResponse::ptr response) = 0;
};

class FunctionServlet : public Servlet {
public:
	using ptr = std::shared_ptr<FunctionServlet>;
	using Func = std::function<void(HttpSession::ptr session
					, HttpRequest::ptr request, HttpResponse::ptr response)>;
	
	FunctionServlet(Func func);
	~FunctionServlet();
	
	void handle(HttpSession::ptr session
			, HttpRequest::ptr request, HttpResponse::ptr response) override;

private:
	Func m_func;
};

class ServletDispatch : public Servlet {
public:
	using ptr = std::shared_ptr<ServletDispatch>;
	using MutexType = RWMutex;

	ServletDispatch();
	~ServletDispatch();
	
	void handle(HttpSession::ptr session
			, HttpRequest::ptr request, HttpResponse::ptr response) override;
	
	bool addExactServlet(const std::string& uri, Servlet::ptr servlet);
	bool addExactServlet(const std::string& uri, FunctionServlet::Func func);
	bool setExactServlet(const std::string& uri, Servlet::ptr servlet);
	bool setExactServlet(const std::string& uri, FunctionServlet::Func func);
	bool delExactServlet(const std::string& uri);
	Servlet::ptr findExactServlet(const std::string& uri);
	
	bool addGlobServlet(const std::string& uri, Servlet::ptr servlet);
	bool addGlobServlet(const std::string& uri, FunctionServlet::Func func);
	bool setGlobServlet(const std::string& uri, Servlet::ptr servlet);
	bool setGlobServlet(const std::string& uri, FunctionServlet::Func func);
	bool delGlobServlet(const std::string& uri);
	Servlet::ptr findGlobServletServlet(const std::string& uri);
	
	Servlet::ptr uriMatchServlet(const std::string& uri);
	
	void setDefaultServlet(Servlet::ptr servlet);
	Servlet::ptr getDefaultServlet();

private:
	mutable MutexType m_rwmtx;
	std::map<std::string, Servlet::ptr> m_exactServlets;
	std::map<std::string, Servlet::ptr> m_globServlets;
	Servlet::ptr m_defaultServlet;
};

class NotFoundServlet : public Servlet {
public:
	using ptr = std::shared_ptr<NotFoundServlet>;

	NotFoundServlet(const std::string& server);
	~NotFoundServlet();

	void handle(HttpSession::ptr session
			, HttpRequest::ptr request, HttpResponse::ptr response) override;
private:
	std::string m_server;
};

}
}

#endif // ! __WEBSERVER_HTTP_SERVLET_H__
