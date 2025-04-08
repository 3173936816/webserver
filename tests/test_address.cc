#include "webserver.h"

using namespace webserver;

void test_ipv4() {
	uint8_t addr[4] = {0xc0, 0x7b, 0x19, 0x29};

	IPv4Address::ptr ipv4 = std::make_shared<IPv4Address>(addr, 9950);
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv4->visual();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv4->getBoradcast(24)->visual();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv4->getNetworkSegment(24)->visual();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv4->getSubnetMask(24)->visual();
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv4->getPort();
	ipv4->setPort(9960);
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv4->getPort();
}

void test_ipv6() {
	IPv6Address::ptr ipv6
		 = std::make_shared<IPv6Address>("fe80::4b57:85a2:3636:3809", 9950);

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv6->visual();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv6->getBoradcast(32)->visual();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv6->getNetworkSegment(32)->visual();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv6->getSubnetMask(32)->visual();
	
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv6->getPort();
	ipv6->setPort(9960);
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << ipv6->getPort();
}

void test_unix() {
	UnixAddress::ptr unix
		 = std::make_shared<UnixAddress>("lcy/ppp/fggss.s");

	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << unix->visual();
	WEBSERVER_LOG_INFO(WEBSERVER_LOGMGR_ROOT()) << unix->getActualLength();
}

void test_resoluteDomainName() {
	std::vector<IPAddress::ptr> vec;
	IPAddress::ResoluteDomainName(vec, "www.baidu.com", "80");
	
	for(auto& it : vec) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT())
			<< it->visual();
	}
	std::cout << std::endl << std::endl;	


	std::vector<IPv6Address::ptr> tmp_vec;
	IPv6Address::ResoluteAllTCP(tmp_vec, "www.baidu.com", "80");
	
	for(auto& it : tmp_vec) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT())
			<< it->visual();
	}
}

void test_getifaddr() {
	std::multimap<std::string, std::pair<IPAddress::ptr, uint32_t> > ifaddresses;
	IPAddress::GetIfAddresses(ifaddresses, Domain::IPv6);
	
	for(auto& it : ifaddresses) {
		WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT())
			<< "name: " << it.first 
			<< "  addr: " << it.second.first->visual()
			<< "  prefix_len: " << it.second.second;
	}
	
	std::pair<IPv6Address::ptr, uint32_t> ipv6 = IPv6Address::GetAnyIfAddress();
	WEBSERVER_LOG_ERROR(WEBSERVER_LOGMGR_ROOT()) << ipv6.first->visual();
	
}

int main() {
	//test_ipv4();
	//test_ipv6();
	//test_unix();
	//test_resoluteDomainName();
	test_getifaddr();

	return 0;
}
