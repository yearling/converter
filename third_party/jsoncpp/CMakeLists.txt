cmake_minimum_required(VERSION 3.1.0)

file(GLOB_RECURSE LIB_INCLUDES "./*.h")
file(GLOB_RECURSE LIB_SRCS 	"./*.cpp")
						
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/third_party/libs)
add_library(jsoncpp STATIC ${LIB_SRCS} ${LIB_INCLUDES})

target_include_directories(jsoncpp PUBLIC "./")
