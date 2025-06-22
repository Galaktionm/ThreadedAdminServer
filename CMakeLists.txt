cmake_minimum_required(VERSION 3.31)
project(ThreadedAdminServer C)

set(CMAKE_C_STANDARD 11)

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)

include_directories(${JANSSON_INCLUDE_DIRS})
link_directories(${JANSSON_LIBRARY_DIRS})

add_executable(ThreadedAdminServer main.c
        server.h
        server.c
        thread_pool.c
        thread_pool.h
        request.h
        request.c
        response.c
        response.h
        signal.c
        signal.h
        auth.c
        auth.h
        base64.c
        base64.h
        service_manager.c
        service_manager.h)

target_link_libraries(ThreadedAdminServer PRIVATE OpenSSL::SSL OpenSSL::Crypto ${JANSSON_LIBRARIES} pthread)
