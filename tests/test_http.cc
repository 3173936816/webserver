#include "webserver.h"

using namespace webserver;

void test_convert() {
	std::cout << http::HttpStatusToString(http::HttpStatus::OK) << std::endl;
	std::cout << (int)http::StringToHttpStatus("ok") << std::endl;
	std::cout << http::GetHttpStatusDescription(
			http::HttpStatus::NON_AUTHORITATIVE_INFORMATION) << std::endl;
	
	std::cout << http::HttpMethodToString(http::HttpMethod::GET) << std::endl;
	std::cout << (int)http::StringToHttpMethod("get") << std::endl;
}

void test_httpRequest() {
	http::HttpRequest::ptr request 
			= std::make_shared<http::HttpRequest>(http::HttpMethod::GET, 0x10);

	request->setPath("/baidu");
	request->setQuery("key_1=value_1&key_2=value_2");
	request->setFragment("fragment");
	request->addHeaderField("Connection", "close");

	std::cout << request->getRequest() << std::endl;
}

void test_httpResponse() {
	std::ifstream ifs;
	ifs.open("../tests/html/hello_world.html");
	
	std::string context;
	std::string tmp;
	while(ifs >> tmp) {
		context += tmp;
	}
	ifs.close();
	
	http::HttpResponse::ptr response
			= std::make_shared<http::HttpResponse>(http::HttpStatus::OK, 0x10);
	response->addHeaderField("Connection", "keep alive");	
	response->addHeaderField("Content-Type", "text/html");	
	response->addHeaderField("Content-Length", std::to_string(context.size()));	
	response->setBody(context);

	Socket::ptr server_sock = Socket::CreateIPv4TCPSocket();
	IPv4Address::ptr server_addr = IPv4Address::GetAnyIfAddress().first;
	server_addr->setPort(9920);
	std::cout << server_addr->visual() << std::endl;
	
	server_sock->bind(server_addr);
	server_sock->listen(1024);
	
	Socket::ptr client = server_sock->accept();
	if(!client) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT())
			<< server_sock->getErrorMsg();
		return;
	}
	
	std::string response_str = response->getResponse();

	ByteArray::ptr by = std::make_shared<ByteArray>();
	by->writeStringAsC(response_str);
	std::vector<struct iovec> vec;
	by->getReadBuffers(vec);	

	ssize_t size = client->writev(&vec[0], vec.size());
	std::cout << vec[0].iov_len << std::endl;
	std::cout << response_str.size() << std::endl;
	std::cout << size << std::endl;
}

void test_size() {
	Config::EffectConfig("/home/lcy/projectsMySelf/MyWebserver/conf/module_confs.yml");
	std::cout << http::GetHttpRequestBuffSize() << std::endl;
	std::cout << http::GetHttpRequestMaxBodySize() << std::endl;
	std::cout << http::GetHttpResponseBuffSize() << std::endl;
	std::cout << http::GetHttpResponseMaxBodySize() << std::endl;
}

int main() {
	//test_convert();
	//test_httpRequest();
	//test_httpResponse();
	test_size();

	return 0;
}
