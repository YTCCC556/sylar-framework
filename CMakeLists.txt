cmake_minimum_required(VERSION 3.25)
project(sylar-framework)


set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

# 引入boost
find_package(Boost COMPONENTS REQUIRED)
if (Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
endif ()

set(LIB_SRC
        src/log.cpp
        src/util.cpp
        src/config.cpp
        src/thread.cpp
        src/macro.cpp
)

add_library(framework SHARED ${LIB_SRC})
target_link_libraries(framework ${Boost_LIBRARIES} yaml-cpp pthread dl)

file(GLOB CPP_SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp)
foreach (v ${CPP_SRC_LIST})
    string(REGEX MATCH "tests/.*" relative_path ${v})
    string(REGEX REPLACE "tests/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})
    message(STATUS "找到测试文件：${v}")
    add_executable(${target_name} ${v})
    target_link_libraries(${target_name} framework)
endforeach ()

#set(LIB_SRC
#        src/log.cpp
#        src/util.cpp
#        src/config.cpp
#)
#
#add_library(ytccc SHARED ${LIB_SRC})

#add_executable(test_log tests/test_log.cpp)
#add_dependencies(test_log ytccc)
#target_link_libraries(test_log ytccc)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

