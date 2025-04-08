#ifndef __WEBSERVER_NONCOPYABLE_H__
#define __WEBSERVER_NONCOPYABLE_H__

namespace webserver {

class Noncopyable {
public:

	Noncopyable() = default;	
	~Noncopyable() = default;

	Noncopyable(const Noncopyable& rhs) = delete;
	Noncopyable& operator=(const Noncopyable& rhs) = delete;

	Noncopyable(Noncopyable&& rhs) = delete;
	Noncopyable& operator=(Noncopyable&& rhs) = delete;
};

}	//namespace webserver

#endif // !__WEBSERVER_NONCOPYABLE_H__i
