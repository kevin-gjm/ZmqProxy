cmake_minimum_required(VERSION 2.6)

project(Proxy)


if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
message (FATAL_ERROR "CMake generation is not allowed within the source directory!
Remove the CMakeCache.txt file and try again from another folder, e.g.:

   rm CMakeCache.txt
   mkdir build
   cd build
   cmake ..
")
endif ()

set(C_BUILD_FLAGS "-Wall -std=gnu99")

set(CMAKE_C_FLAGS "${C_BUILD_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${C_BUILD_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_MINSIZEREL "${C_BUILD_FLAGS} ${CMAKE_C_FLAGS_MINSIZEREL}")
set(CMAKE_C_FLAGS_RELEASE "${C_BUILD_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${C_BUILD_FLAGS} ${CMAKE_C_FLAGS_RELWITHDEBINFO}")


set(CXX_FLAGS
 # -DVALGRIND
 # -DMUDUO_STD_STRING
 -DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
 #-Werror
 #-Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 # -MMD
 -std=c++11
 #-rdynamic
)
string(REPLACE ";" " " CXX_BUILD_FLAGS "${CXX_FLAGS}")



set(CMAKE_CXX_FLAGS "${CXX_BUILD_FLAGS} ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CXX_BUILD_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CXX_BUILD_FLAGS} ${CMAKE_CXX_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_RELEASE "${CXX_BUILD_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CXX_BUILD_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")


IF (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "")
ADD_DEFINITIONS(-g -DDEBUG)
ENDIF ()



include_directories(./src)
include_directories(./Smart_Park_Public_Include)

set(SRC_FILES
src/main.cc
src/zmq_server.h
src/zmq_server.cc
src/client_info.h
src/ini.c
src/ini.h
src/zmq.hpp
src/zmq_addon.hpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}  
glog
zmq
${MYSQL_LIBRARIES}
)

