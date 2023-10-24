cmake_minimum_required(VERSION 3.25)
project(sylar-framework)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

set(LIB_SRC
        src/log.cpp
        src/util.cpp
)

add_library(sylar-framework SHARED ${LIB_SRC})


add_executable(test_log tests/tests_log.cpp)
add_dependencies(test_log sylar-framework)
target_link_libraries(test_log sylar-framework)

add_executable(test_util tests/tests_log.cpp)
add_dependencies(test_util sylar-framework)
target_link_libraries(test_util sylar-framework)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
