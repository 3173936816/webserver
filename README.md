高性能高并发网络服务器

模块介绍:

## 线程同步模块(synchronism)
使用C库封装了信号量, 互斥锁(支持递归锁), 读写锁, 自旋锁, 条件变量以及屏障, 同时提供了类似于std::lock_guard的RAII类,提供了三种锁策略:
TryLock : 尝试加锁, 成功后析构时解锁, Locked : 默认加锁, 析构时解锁, KeepLock : 仅保存锁.

为什么c++11已经有了std::mutex等并发支持库还要进行封装? 
  由于c++11的并发支持库有限, 例如没有读写锁(C++14), 但是在高并发的环境下读写锁非常常见, 
  尤其是在读多写少的情况下, 读写锁可以提供不错的性能提升, 而且C++的std并发库也是通过C库进行封装的(要链接-lpthread)

## 日志模块(log)
日志类似于Log4cpp, 支持流式风格的日志和格式化风格的日志, 支持用户自定义日志风格, 支持DEBUG, INFO, WARN, ERROR, FATAL日志级别, 
支持日志格式的解析, 用户可以根据自己喜欢的日志风格进行指定, 

  %o owner            -日志归属用户       %L Level            -日志级别
  %f file             -文件名称           %l line             -行号
  %t Tid              -线程tid            %N Name             -线程名称
  %c corottineId      -协程id             %D Time             -时间
  %m message          -日志信息           %s string           -自定义信息
  %T \t               -制表符             %n std::endl        -换行

默认格式为 " %D{%Y-%m-%d %H:%M:%S}%T%t%T%c%T%N%T[%L]%T[%o]%T%f:%l%T%m%n"
输出例示 : 
2025-04-09 06:48:05	3390	3390	test_log	[DEBUG]	[system]	./tests/test_log.cc:89	webserver_log_level
2025-04-09 06:48:05	3390	3390	test_log	[FATAL]	[system]	./tests/test_log.cc:90	webserver_log_level fatal

同时支持多种风格的日志类型 :
''' 
            |-------- 控制台日志
日志类型 ---|                          |----------  单一文件日志
            |-------- 文件日志 --------|----------  时间轮转式日志 : 根据日期进行日志记录, 可以设置最多保存的日志数量, 文件命名 --- example_2025-01-01.log
                                       |----------  文件大小轮转日志 : 根据文件的大小进行日志记录,  可以设置最多保存的日志数量和每个日志文件的大小, 文件命名 ---- example_1.log
'''

## 配置模块(config)
使用'yaml'格式文件作为配置文件, 支持yaml文件的序列化和反序列化, 使用配置系统时,只需要偏特化LexicalCast类即可,
提供STL容器(vector, list, set, unordered_map, unoreder_set等等)的序列化支持, 配置使用方式例如:

'''
using Type = Map<std::string, std::string>;
static ConfigVar<Type>::ptr Coroutine_Config_Var = 
        ConfigSingleton::GetInstance()->addConfigVar<Type>("coroutine", 
            Type{{"stackSize", "1024 * 1024"}},  "this configvar defined coroutine stack size");  
'''

对应的配置文件:
'''
 - coroutine:
       stackSize : 1024 * 1024
'''
该配置定义了协程栈的大小

'''
- logs :
    - owner : root
      level : DEBUG
      format : "%D{%Y-%m-%d %H:%M:%S}%T%t%T%c%T%N%T[%L]%T[%o]%T%f:%l%T%m%n"
      ConsoleLogAppender :
          - level : DEBUG
            format : default
      SizeRotateFileAppender :
          - level : WARN
            format : default
            path : "../logs/root/root.log"
            maxCount : 8
            maxSize : 512 * 12
'''
该配置定义了root日志器的配置

## 线程模块(thread)
定义了Thread类, 提供线程支持, 其实也可以使用std标准库的std::thread替代, 但在本项目中日志有线程名称, std::thread中只有pthread_t id(进程级别id)
也没有 线程的tid(内核级别id)获取方式, 所以选择自己封装一个, 同时也可以提升一下自己的编程能力


## 协程模块(coroutine)
使用Linux提供的ucontext库封装的有栈非对称协程, 协程也称为用户级线程, 但是比线程更轻量级, 切换由用户控制,无需内核调度, 协程可以使得异步代码的编写
更加方便, 本项目的协程和std::thread的风格相识:

'''
Coroutine::ptr co = std::make_shared<Coroutine>();
co->swapIn();    ------> 由主协程切入该协程
now_coroutine::SwapOut();  ------> 由当前协程切回主协程
'''


## 调度器模块(scheduler)
本质上是个线程池, 用于协程任务的调度和执行, 可以指定任务在哪个线程执行, 也支持一个线程在多个线程中切换执行, 是一个N-M的调度模型


## Input/Output模块(IO)
使用IOBase作为IO的基类, 目前实现了epollio, 采用Linux的多路IO模型epoll作为底层, 支持毫秒级的定时器, 支持一次定时和循环定时, 支持事件的增加,删除, 触发功能


## 异步(async)
对Linux下常用的API进行了异步封装, 如: async::Accept, async::Connect, async:Read, async::Write, ...., async::Setsockopt等等, 和Linux的原始接口相同,异步的开启是线程粒度的, 在IO调度器中, 
这些函数会展现出异步特性, 一个任务阻塞不会影响另一个任务的执行, 而在调度器线程外, 这些函数效果和Linux原始的API效果一致


## 网络套接字(socket)
统一分装了Socket的地址类, 支持IPv4, IPv6, Unix地址的解析, 提供网络掩码, 网段和广播地址的获取, 支持域名(www.baidu.comdu.com)解析和本机网卡接口地址的解析
                    Address ----------UnixAddress
                       |
                   IPAddress
                       |
                ----------------
                |              |
          IPv4Address      IPv6Address

同时封装了socket类, 支持所有socketAPI功能, 同时支持connect_timeout超时连接


## ByteArray
ByteArray 二进制序列化模块, 实现常用数据类型uint8_t, int8_t, ..., uint64_t, int64_t的相关读写方式, 采用google的zigzag编码 和 varint可变长度编码实现基本数据类型的字节压缩,
这样可以减少数据的传输量提高网络通信的性能, 同时提供和socketAPI 的struct iovec 的转化接口, 还支持数据的文件序列化和反序列化(二进制和十六进制)


## 服务器模块(server)
以Server类作为服务器的基类, 支持一个地址和多个地址的快速绑定bind, listen, 地址复用一次做完, 支持TCP, UDP协议, 支持dispatch事件循环, 支持信号事件的处理, 
具体服务器只需继承该类实现其虚函数接口即可快速实现, 目前实现了HttpServer

## http协议
 - 封装了HttpRequest和HttpResponse类, 用于表示http协议, HttpParser采用ragel有限状态机生成代码(性能堪比汇编语言), 用于实现http协议的解析, 同时设定http的一些指标,
如头部大小, body数据长度

 - 实现了HttpSession, 支持单个socket的数据接收和发送, 处理一些非法的http请求, 例如头部太长的http请求

 - 实现了HttpServer

## Servlet
仿照java的servlet, 实现http的系列系列接口,  实现了ServletDispatch, FunctionServlet, NotFoundServlet等基础服务, 支持uri的精确匹配和模糊匹配
