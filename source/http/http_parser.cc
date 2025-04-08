#include "http_parser.h"

#include <strings.h>
#include <iostream>

namespace webserver {
namespace http {

static thread_local bool request_has_error = false;
static thread_local bool response_has_error = false;

//	HttpRequestParser
static void request_http_field_cb(void *data, const char *field
		 , size_t flen, const char *value, size_t vlen) {
	if(!flen || !vlen) {
		request_has_error = true;
		return;
	}
	HttpRequest* request = static_cast<HttpRequest*>(data);
	request->addHeaderField(std::string(field, flen), std::string(value, vlen));
}

static void request_request_method_cb(void *data, const char *at, size_t length) {
	HttpRequest* request = static_cast<HttpRequest*>(data);
	
	HttpMethod method = StringToHttpMethod(std::string(at, length));
	if(method == HttpMethod::INVALID) {
		request_has_error = true;
		return;
	}
	request->setMethod(method);
}

static void request_request_uri_cb(void *data, const char *at, size_t length) {
}

static void request_fragment_cb(void *data, const char *at, size_t length) {
	HttpRequest* request = static_cast<HttpRequest*>(data);
	request->setFragment(std::string(at, length));
}

static void request_request_path_cb(void *data, const char *at, size_t length) {
	if(!length) {
		request_has_error = true;
		return;
	}
	HttpRequest* request = static_cast<HttpRequest*>(data);
	request->setPath(std::string(at, length));
}

static void request_query_string_cb(void *data, const char *at, size_t length) {
	HttpRequest* request = static_cast<HttpRequest*>(data);
	request->setQuery(std::string(at, length));
}

static void request_http_version_cb(void *data, const char *at, size_t length) {
	HttpRequest* request = static_cast<HttpRequest*>(data);
	std::string version(at, length);
	if(strcasecmp(version.c_str(), "HTTP/1.0") == 0) {
		request->setVersion(0x10);
	} else if(strcasecmp(version.c_str(), "HTTP/1.1") == 0) {
		request->setVersion(0x11);
	} else {
		request_has_error = true;
	}
}

static void request_header_done_cb(void *data, const char *at, size_t length) {
}

HttpRequestParser::HttpRequestParser()
	: m_parser{}, m_httpRequest{} {
	
	http_parser_init(&m_parser);
	m_parser.http_field = request_http_field_cb;
	m_parser.request_method = request_request_method_cb;
	m_parser.request_uri = request_request_uri_cb;
	m_parser.fragment = request_fragment_cb;
	m_parser.request_path = request_request_path_cb;
	m_parser.query_string = request_query_string_cb;
	m_parser.http_version = request_http_version_cb;
	m_parser.header_done = request_header_done_cb;
}

HttpRequestParser::~HttpRequestParser() = default;

size_t HttpRequestParser::execute(const std::string& data, size_t off) {
	m_httpRequest = std::make_shared<HttpRequest>();
	m_parser.data = m_httpRequest.get();
	request_has_error = false;

	return http_parser_execute(&m_parser, data.c_str(), data.size(), off); 
}

bool HttpRequestParser::has_error() const {
	return request_has_error || http_parser_has_error(&m_parser);
}

bool HttpRequestParser::is_finished() const {
	return http_parser_is_finished(&m_parser);
}

// HttpResponseParser

void response_http_field_cb(void *data, const char *field
		, size_t flen, const char *value, size_t vlen) {
	if(!flen || !vlen) {
		response_has_error = true;
		return;
	}
	HttpResponse* response = static_cast<HttpResponse*>(data);
	response->addHeaderField(std::string(field, flen), std::string(value, vlen));
}

void response_reason_phrase_cb(void *data, const char *at, size_t length) {	
}

void response_status_code_cb(void *data, const char *at, size_t length) {
	HttpResponse* response = static_cast<HttpResponse*>(data);

	int status = std::stoi(std::string(at, length));
	if(status > 511 || status < 100) {
		response_has_error = true;
		return;
	}
	response->setStatus(static_cast<HttpStatus>(status));
}

void response_chunk_size_cb(void *data, const char *at, size_t length) {
}

void response_http_version_cb(void *data, const char *at, size_t length) {
	HttpResponse* response = static_cast<HttpResponse*>(data);
	std::string version(at, length);
	if(strcasecmp(version.c_str(), "HTTP/1.0") == 0) {
		response->setVersion(0x10);
	} else if(strcasecmp(version.c_str(), "HTTP/1.1") == 0) {
		response->setVersion(0x11);
	} else {
		response_has_error = true;
	}
}

void response_header_done_cb(void *data, const char *at, size_t length) {
}

void response_last_chunk_cb(void *data, const char *at, size_t length) {
}

HttpResponseParser::HttpResponseParser()
	: m_parser{}, m_httpResponse{} {
	httpclient_parser_init(&m_parser);
	
 	m_parser.http_field = response_http_field_cb;
 	m_parser.reason_phrase = response_reason_phrase_cb;
 	m_parser.status_code = response_status_code_cb;
 	m_parser.chunk_size = response_chunk_size_cb;
 	m_parser.http_version = response_http_version_cb;
 	m_parser.header_done = response_header_done_cb;
 	m_parser.last_chunk = response_last_chunk_cb;
}

HttpResponseParser::~HttpResponseParser() = default;
	
size_t HttpResponseParser::execute(const std::string& data, size_t off) {
	m_httpResponse = std::make_shared<HttpResponse>();
	m_parser.data = m_httpResponse.get();
	response_has_error = false;
	
	return httpclient_parser_execute(&m_parser, data.c_str(), data.size(), off); 
}

bool HttpResponseParser::has_error() const {
	return response_has_error || httpclient_parser_has_error(&m_parser);
}

bool HttpResponseParser::is_finished() const {
	return httpclient_parser_is_finished(&m_parser);
}

}	// namespace http
}	// namespace webserver
