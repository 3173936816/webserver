#ifndef __WEBSERVER_HTTP_HTTP_PARSER_H__
#define __WEBSERVER_HTTP_HTTP_PARSER_H__

#include <memory>
#include <string>

#include "http.h"
#include "ragel/http11_common.h"
#include "ragel/http11_parser.h"
#include "ragel/httpclient_parser.h"
#include "../source/noncopyable.h"

namespace webserver {
namespace http {

class HttpRequestParser : Noncopyable {
public:
	using ptr = std::shared_ptr<HttpRequestParser>;
	
	HttpRequestParser();
	~HttpRequestParser();
	
	size_t execute(const std::string& data, size_t off = 0);
	bool has_error() const;
	bool is_finished() const;
	
	HttpRequest::ptr getData() { return m_httpRequest; }

private:
	struct http_parser m_parser;
	HttpRequest::ptr m_httpRequest;
};


class HttpResponseParser : Noncopyable {
public:
	using ptr = std::shared_ptr<HttpResponseParser>;
	
	HttpResponseParser();
	~HttpResponseParser();
	
	size_t execute(const std::string& data, size_t off = 0);
	bool has_error() const;
	bool is_finished() const;
	
	HttpResponse::ptr getData() { return m_httpResponse; }

private:
	struct httpclient_parser m_parser;
	HttpResponse::ptr m_httpResponse;
};

}	// namespace http
}	// namespace webserver

#endif // ! __WEBSERVER_HTTP_HTTP_PARSER_H__
