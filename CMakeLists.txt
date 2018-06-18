cmake_minimum_required(VERSION 3.9)
project(httpsrv)

# set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -g -Wall  -Wextra -Wno-unused-parameter -Wno-unused-variable")
add_executable(httpsrv httpsrv.c timer.c pq.c handle_request.c  thpool.c fd.c global.c picohttpparser.c  util.c)


# unused parameter 'i' [-Werror,-Wunused-parameter]