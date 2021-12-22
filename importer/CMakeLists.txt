cmake_minimum_required(VERSION 3.1.0)

file(GLOB_RECURSE LIB_INCLUDES "include/*.h") 
file(GLOB_RECURSE LIB_SRCS 	"src/*.cpp")
						
# Ìí¼Óc++±ê×¼
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /GR")
else(MSVC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif(MSVC)
add_compile_definitions(FBXSDK_SHARED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fbxsdk/include
					${CMAKE_CURRENT_SOURCE_DIR}/../engine/include)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../libs)
add_library(fbximporter STATIC ${LIB_SRCS} ${LIB_INCLUDES})
target_include_directories(fbximporter PUBLIC "include")
