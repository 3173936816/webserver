高性能高并发网络服务器

模块介绍:

## 线程同步模块
使用C库封装了信号量, 互斥锁(支持递归锁), 读写锁, 自旋锁, 条件变量以及屏障, 同时提供了类似于std::lock_guard的RAII类,提供了三种锁策略:
TryLock : 尝试加锁, 成功后析构时解锁, Locked : 默认加锁, 析构时解锁, KeepLock : 仅保存锁.

为什么c++11已经有了std::mutex等并发支持库还要进行封装? 
  由于c++11的并发支持库有限, 例如没有读写锁(C++14), 但是在高并发的环境下读写锁非常常见, \
  尤其是在读多写少的情况下, 读写锁可以提供不错的性能提升, 而且C++的std并发库也是通过C库进行封装的(要链接-lpthread)

## 日志模块
日志类似于Log4cpp, 支持流式风格的日志和格式化风格的日志, 支持用户自定义日志风格, 支持DEBUG, INFO, WARN, ERROR, FATAL日志级别, 
支持日志格式的解析, 用户可以根据自己喜欢的日志风格进行指定, 


  %o owner            -日志归属用户       %L Level            -日志级别
  %f file             -文件名称           %l line             -行号
  %t Tid              -线程tid            %N Name             -线程名称
  %c corottineId      -协程id             %D Time             -时间
  %m message          -日志信息           %s string           -自定义信息
  %T \t               -制表符             %n std::endl        -换行
 

默认格式为 " %D{%Y-%m-%d %H:%M:%S}%T%t%T%c%T%N%T[%L]%T[%o]%T%f:%l%T%m%n "
输出例示 : 
2025-04-09 06:48:05	3390	3390	test_log	[DEBUG]	[system]	./tests/test_log.cc:89	webserver_log_level
2025-04-09 06:48:05	3390	3390	test_log	[FATAL]	[system]	./tests/test_log.cc:90	webserver_log_level fatal

同时支持多种风格的日志类型 :
| 
            |-------- 控制台日志
日志类型 ---|                          |----------  单一文件日志
            |-------- 文件日志 --------|----------  时间轮转式日志 : 根据日期进行日志记录, 可以设置最多保存的日志数量, 文件命名 --- example_2025-01-01.log
                                       |----------  文件大小轮转日志 : 根据文件的大小进行日志记录,  可以设置最多保存的日志数量和每个日志文件的大小, 文件命名 ---- example_1.log
