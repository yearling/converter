cmake_minimum_required(VERSION 3.12.1)
project(demo)

SET(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
SET(FBX_BIN ${THIRD_PARTY}/fbxsdk/lib/vs2017/x64)
SET(THIRD_PARTY_INCLUDE ${THIRD_PARTY}/fbxsdk/include)
# cpp files
file(GLOB_RECURSE SRC_LIST "src/*.cpp" "src/*.h" )
#message( "${SRC_LIST}" )
#include
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${THIRD_PARTY_INCLUDE})
file(GLOB_RECURSE HEAD_LIST "include/*.h")

add_compile_definitions(FBXSDK_SHARED)

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /GR")
else(MSVC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif(MSVC)
add_executable(demo ${SRC_LIST} ${HEAD_LIST})

source_group("Public Headers" FILES ${HEAD_LIST})

#link target
target_link_libraries(
    demo debug  ${THIRD_PARTY}/fbxsdk/lib/vs2017/x64/debug/libfbxsdk.lib   optimized ${THIRD_PARTY}/fbxsdk/lib/vs2017/x64/release/libfbxsdk.lib 
)
# target_link_libraries(demo $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/../editor/3rdlib/Debug/LinearMath.lib> $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/../editor/3rdlib/Release/LinearMath.lib>)

# post build
add_custom_command(
		TARGET demo POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${FBX_BIN}/debug/libfbxsdk.dll> $<$<CONFIG:Release>:${FBX_BIN}/release/libfbxsdk.dll> 	$<TARGET_FILE_DIR:demo>
)