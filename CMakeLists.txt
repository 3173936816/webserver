#CMake 版本要求
cmake_minimum_required(VERSION 3.14.0)

# 定义项目名称
project(WebServer)

# 设置编译器选项，优化环境变量使用方式
if(DEFINED ENV{CXXFLAGS})
    set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated")
else()
    set(CMAKE_CXX_FLAGS "-rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated")
endif()

# 使用相对路径
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 让 __FILE__ 输出相对于项目根目录的路径（如 ./src/file.c）
    add_compile_options("-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=./")
endif()

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/source)

# 搜索源文件，使用file(GLOB)精准匹配
#file(GLOB SRC_LIST ${PROJECT_SOURCE_DIR}/source/*.cc)

set(SRC_LIST
	source/exception.cc
	source/time_structure.cc
	source/synchronism.cc
	source/log.cc
	source/util.cc
	source/config.cc
	source/thread.cc
	source/coroutine.cc
	source/tools/scheduler.cc
	source/tools/timer.cc
	source/iobase.cc
	source/epollio.cc
	source/tools/sockfdinfo.cc
	source/async.cc
	source/endian_convert.cc
	source/address.cc
	source/socket.cc
	source/bytearray.cc
	source/http/http.cc
	source/http/ragel/http11_parser.cc
	source/http/ragel/httpclient_parser.cc
	source/http/http_parser.cc
	source/server.cc
	source/http/http_session.cc
	source/http/http_servlet.cc
	source/http/http_server.cc
)

# 设置库输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 生成共享库
add_library(WebServer SHARED ${SRC_LIST})
add_library(ConfigSystem SHARED source/config.cc)
# add_library(WebServer STATIC ${SRC_LIST})

# 包含库目录
link_directories(${PROJECT_SOURCE_DIR}/lib)

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#连接库
set(LINK_LIBS
	pthread 
	WebServer
	yaml-cpp
	boost_thread
)

# 生成可执行文件，并按规范链接库
#add_executable(test_synchronism ${PROJECT_SOURCE_DIR}/tests/test_synchronism.cc)
#target_link_libraries(test_synchronism ${LINK_LIBS})

#add_executable(test_log ${PROJECT_SOURCE_DIR}/tests/test_log.cc)
#target_link_libraries(test_log ${LINK_LIBS})

#add_executable(test_config ${PROJECT_SOURCE_DIR}/tests/test_config.cc)
#target_link_libraries(test_config ${LINK_LIBS})

#add_executable(test_thread ${PROJECT_SOURCE_DIR}/tests/test_thread.cc)
#target_link_libraries(test_thread ${LINK_LIBS})

#add_executable(test_coroutine ${PROJECT_SOURCE_DIR}/tests/test_coroutine.cc)
#target_link_libraries(test_coroutine ${LINK_LIBS})

#add_executable(test_scheduler ${PROJECT_SOURCE_DIR}/tests/test_scheduler.cc)
#target_link_libraries(test_scheduler ${LINK_LIBS})
#
#add_executable(test_epollio ${PROJECT_SOURCE_DIR}/tests/test_epollio.cc)
#target_link_libraries(test_epollio ${LINK_LIBS})
#
add_executable(test_timer ${PROJECT_SOURCE_DIR}/tests/test_timer.cc)
target_link_libraries(test_timer ${LINK_LIBS})

#add_executable(test_async ${PROJECT_SOURCE_DIR}/tests/test_async.cc)
#target_link_libraries(test_async ${LINK_LIBS})

#add_executable(test_address ${PROJECT_SOURCE_DIR}/tests/test_address.cc)
#target_link_libraries(test_address ${LINK_LIBS})

#add_executable(test_socket ${PROJECT_SOURCE_DIR}/tests/test_socket.cc)
#target_link_libraries(test_socket ${LINK_LIBS})

#add_executable(test_bytearray ${PROJECT_SOURCE_DIR}/tests/test_bytearray.cc)
#target_link_libraries(test_bytearray ${LINK_LIBS})

#add_executable(test_http ${PROJECT_SOURCE_DIR}/tests/test_http.cc)
#target_link_libraries(test_http ${LINK_LIBS})

add_executable(test_http_parser ${PROJECT_SOURCE_DIR}/tests/test_http_parser.cc)
target_link_libraries(test_http_parser ${LINK_LIBS})

add_executable(helloWorldServer ${PROJECT_SOURCE_DIR}/source/examples/helloworldServer.cc)
target_link_libraries(helloWorldServer ${LINK_LIBS})

add_executable(httpServer ${PROJECT_SOURCE_DIR}/source/examples/httpServer.cc)
target_link_libraries(httpServer ${LINK_LIBS})
