#ifndef __WEBSERVER_SINGLETON_H__
#define __WEBSERVER_SINGLETON_H__

#include <memory>

namespace webserver {

template <class T>
class Singleton {
public:
	static T* GetInstance() {
		static typename T::ptr instance = std::make_shared<T>();
		return instance.get();
	}
};

template <class T>
class SingletonPtr {
public:
	static typename T::ptr GetInstance() {
		static typename T::ptr instance = std::make_shared<T>();
		return instance;
	}
};

}	// namespace webserver

#endif // !__WEBSERVER_SINGLETON_H__
