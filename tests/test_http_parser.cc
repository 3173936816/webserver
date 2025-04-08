#include "webserver.h"

using namespace webserver;

std::string httpRequest = "POST /kkk?key=value&key=value#fragment HTTP/1.1\r\n"
						  "Connection\r\n\r\n";
						  //"Content-Length: 9\r\n"
						  //"\r\n"
						  //"kkkkkkkkk";

void test_httpRequestParser() {
	http::HttpRequestParser::ptr parser = std::make_shared<http::HttpRequestParser>();
	parser->execute(httpRequest);

	if(parser->has_error()) {
		std::cout << "has error" << std::endl;
		return;
	}
	if(parser->is_finished()) {
		std::cout << parser->getData()->getRequest() << std::endl;
		std::cout << parser->getData()->getQuery() << std::endl;
		std::cout << parser->getData()->getFragment() << std::endl;
	} else {
		std::cout << "no finish" << std::endl;
		return;
	}
}

std::string httpResponse =
	"HTTP/1.1 200 OK\r\n"
	"Accept-Ranges: bytes\r\n"
	"Cache-Control: no-cache\r\n"
	"Connection: keep-alive\r\n"
	"Content-Length: 29506\r\n"
	"Content-Type: text/html\r\n"
	"Date: Sun, 30 Mar 2025 12:57:04 GMT\r\n"
	R"(P3p: CP=" OTI DSP COR IVA OUR IND COM ")""\r\n"
	R"(P3p: CP=" OTI DSP COR IVA OUR IND COM ")""\r\n"
	"Pragma: no-cache\r\n"
	"Server: BWS/1.1\r\n"
	"Set-Cookie: BAIDUID=01D244693646C15A852CA5F4CF2AFEBA:FG=1; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
	"Set-Cookie: BIDUPSID=01D244693646C15A852CA5F4CF2AFEBA; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
	"Set-Cookie: PSTM=1743339424; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
	"Set-Cookie: BAIDUID=01D244693646C15A188387F6B02589E6:FG=1; max-age=31536000; expires=Mon, 30-Mar-26 12:57:04 GMT; domain=.baidu.com; path=/; version=1; comment=bd\r\n"
	"Traceid: 1743339424268017409010731146919429266103\r\n"
	"Vary: Accept-Encoding\r\n"
	"X-Ua-Compatible: IE=Edge,chrome=1\r\n"
	"X-Xss-Protection: 1;mode=block\r\n\r\n";


void test_httpResponseParser() {
	http::HttpResponseParser::ptr parser = std::make_shared<http::HttpResponseParser>();
	parser->execute(httpResponse);

	if(parser->has_error()) {
		std::cout << "has error" << std::endl;
		return;
	}
	if(parser->is_finished()) {
		std::cout << parser->getData()->getResponse() << std::endl;
	} else {
		std::cout << "no finish" << std::endl;
	}
}

int main() {
	test_httpRequestParser();
	//test_httpResponseParser();

	return 0;
}
