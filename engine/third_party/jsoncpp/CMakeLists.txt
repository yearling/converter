cmake_minimum_required(VERSION 3.1.0)

file(GLOB_RECURSE LIB_INCLUDES "include/*.h")
file(GLOB_RECURSE LIB_SRCS 	"src/*.cpp" "src/*.inl")
include_directories( "include")
# Ìí¼Óc++±ê×¼
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /GR")
else(MSVC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif(MSVC)

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/third_party/libs)
add_library(jsoncpp STATIC ${LIB_SRCS} ${LIB_INCLUDES})

target_include_directories(jsoncpp PUBLIC "./")
