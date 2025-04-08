#include "http_session.h"
#include "http_parser.h"
#include "../bytearray.h"
#include "../log.h"
#include "../macro.h"

#include <boost/lexical_cast.hpp>

namespace webserver {
namespace http {

constexpr const char* g_logger = "server";

HttpSession::HttpSession(Socket::ptr client, bool owner)
	: m_owner(owner), m_client(client) {
}

HttpSession::~HttpSession() {
	if(m_owner && m_client 
			&& !m_client->isClosed()) {
		m_client->close();
	}
}

static void TCPSendErrorResponse(Socket::ptr sock, HttpStatus status) {
	HttpResponse::ptr error_response = std::make_shared<HttpResponse>(status);
	std::string error_response_str = error_response->getResponse();
	
	sock->send(error_response_str.c_str(), error_response_str.size());
}

bool HttpSession::recvTCPHttpRequest(HttpRequest::ptr& request) {
	if(!m_client || m_client->isClosed()) {
		return false;
	}
	if(!request) {
		request = std::make_shared<HttpRequest>();
	}
	
	uint64_t http_request_buff_size = GetHttpRequestBuffSize();
	uint64_t http_request_max_body_size = GetHttpRequestMaxBodySize();
	
	HttpRequestParser::ptr request_parser = std::make_shared<HttpRequestParser>();
	
	ByteArray::ptr byteArray = std::make_shared<ByteArray>();
	std::vector<struct iovec> iovecs;
	
	uint64_t has_recv_size = 0;
	ssize_t nparse = 0;
	do {
		iovecs.clear();
		byteArray->getWriteBuffers(iovecs, http_request_buff_size - has_recv_size);
		ssize_t rt = m_client->readv(&iovecs[0], iovecs.size());
		if(rt > 0) {
			byteArray->restorative(rt);
			has_recv_size += rt;
			
			std::string recv_str;
			if(has_recv_size >= http_request_buff_size) {
				recv_str = byteArray->peekString(http_request_buff_size);
			} else {
				recv_str = byteArray->peekString(has_recv_size);
			}

			nparse = request_parser->execute(recv_str);
			if(request_parser->has_error()) {
				TCPSendErrorResponse(m_client, HttpStatus::BAD_REQUEST);
				return false;
			}

			if(!request_parser->is_finished()) {
				if(has_recv_size >= http_request_buff_size) {
					TCPSendErrorResponse(m_client, HttpStatus::REQUEST_HEADER_FIELDS_TOO_LARGE);
					return false;
				}
				continue;
			}
	
			break;

		} else if(rt == 0) {
			return false;
		} else {
			if(errno == ETIMEDOUT) {
				TCPSendErrorResponse(m_client, HttpStatus::REQUEST_TIMEOUT);
				return false;
			} else {
				WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_GET(g_logger))
					<< "HttpSession getHttpRequest -- request readv error"
					<< "  what : " << strerror(errno);
				TCPSendErrorResponse(m_client, HttpStatus::INTERNAL_SERVER_ERROR);
				return false;
			}
		}
	} while(true);
	
	request = request_parser->getData();
	ssize_t content_length = 
			boost::lexical_cast<ssize_t>(request->findHeaderField("Content-Length", "0"));
	if(content_length) {
		if((uint64_t)content_length > http_request_max_body_size) {
			TCPSendErrorResponse(m_client, HttpStatus::PAYLOAD_TOO_LARGE);
			return false;
		}

		byteArray->readString(nparse);

		ssize_t left_body_size = content_length - byteArray->getDataSize();
		do {
			iovecs.clear();
			byteArray->getWriteBuffers(iovecs, left_body_size);
			ssize_t rt = m_client->readv(&iovecs[0], iovecs.size());
			if(rt > 0) {
				byteArray->restorative(rt);
				left_body_size -= rt;

				if(left_body_size == 0) {
					break;
				}

				continue;

			} else if(rt == 0) {
				return false;
			} else {
				WEBSERVER_LOG_WARN(WEBSERVER_LOGMGR_GET(g_logger))
					<< "HttpSession getHttpRequest -- body readv error"
					<< "  what : " << strerror(errno);
				if(errno == ETIMEDOUT) {
					TCPSendErrorResponse(m_client, HttpStatus::REQUEST_TIMEOUT);
					return false;
				} else {
					TCPSendErrorResponse(m_client, HttpStatus::INTERNAL_SERVER_ERROR);
					return false;
				}
			}
		} while(true);

		request->setBody(byteArray->readString(content_length));
	}

	return true;
}

bool HttpSession::sendTCPHttpResponse(HttpResponse::ptr response) {
	if(!response || !m_client 
			|| m_client->isClosed()) {
		return false;
	}
	std::string response_str = response->getResponse();
	size_t size = response_str.size();
	size_t has_send_size = 0;
	do {
		char* ptr = &response_str[0] + has_send_size;
		ssize_t rt = m_client->send(ptr, size - has_send_size);
		if(rt > 0) {
			has_send_size += rt;
			if(has_send_size == size) {
				break;
			}
		} else {
			return false;
		}
	} while(true);

	return true;
}


static void UDPSendErrorResponse(Socket::ptr sock, Address::ptr dest_addr, HttpStatus status) {
	HttpResponse::ptr error_response = std::make_shared<HttpResponse>(status);
	std::string error_response_str = error_response->getResponse();
	
	sock->sendto(error_response_str.c_str(), error_response_str.size(), dest_addr);
}

bool HttpSession::recvUDPHttpRequest(HttpRequest::ptr& request, Address::ptr& src_addr) {
	if(!m_client || m_client->isClosed()) {
		return false;
	}
	if(!request) {
		request = std::make_shared<HttpRequest>();
	}
	if(!src_addr) {
		src_addr = Address::Create(m_client->getDomain());
	}
	
	uint64_t http_request_buff_size = GetHttpRequestBuffSize();
	uint64_t http_request_max_body_size = GetHttpRequestMaxBodySize();
	uint64_t max_size = http_request_buff_size + http_request_max_body_size;

	HttpRequestParser::ptr request_parser = std::make_shared<HttpRequestParser>();

	std::string request_str;
	request_str.resize(max_size);
	
	ssize_t rt = m_client->recvfrom(&request_str[0], max_size, src_addr);
	ssize_t nparse = 0;
	if(rt > 0) {
		nparse = request_parser->execute(request_str);
		if(request_parser->has_error()
				|| !request_parser->is_finished()) {

			UDPSendErrorResponse(m_client, src_addr, HttpStatus::BAD_REQUEST);
			return false;
		}	

		request = request_parser->getData();

		ssize_t content_length = 
				boost::lexical_cast<ssize_t>(request->findHeaderField("Content-Length", "0"));
		if(content_length) {
			request->setBody(std::string(request_str.substr(nparse, rt - nparse)));
		}
	} else {
		UDPSendErrorResponse(m_client, src_addr, HttpStatus::BAD_REQUEST);
		return false;
	}

	return true;
}

bool HttpSession::sendUDPHttpResponse(HttpResponse::ptr response, Address::ptr dest_addr) {
	if(!response 
			|| !dest_addr || !m_client 
			|| m_client->isClosed()) {
		return false;
	}
	
	std::string response_str = response->getResponse();
	ssize_t size = response_str.size();
	return m_client->sendto(response_str.c_str(), size, dest_addr) == size;
}

Socket::ptr HttpSession::release() {
	return std::move(m_client);
}

bool HttpSession::close() {
	if(!m_client) {
		return false;
	}
	return m_client->close();
}

}	// namespace http
}	// namespace webserver
