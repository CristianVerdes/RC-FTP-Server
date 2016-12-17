cmake_minimum_required(VERSION 3.3)
project(_RC_FTP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
find_package (Threads)
set(SOURCE_FILES main.cpp directories.h directories.cpp createDB.h createDB.cpp)
add_executable(_RC_FTP ${SOURCE_FILES})
target_link_libraries (_RC_FTP ${CMAKE_THREAD_LIBS_INIT})

find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
find_library(SQLITE3_LIBRARY sqlite3)
if(NOT SQLITE3_INCLUDE_DIR)
    message(SEND_ERROR "Failed to find sqlite3.h")
endif()
if(NOT SQLITE3_LIBRARY)
    message(SEND_ERROR "Failed to find the sqlite3 library")
endif()

target_link_libraries(_RC_FTP
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${SQLITE3_LIBRARY}
        ${THREAD_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        rt)