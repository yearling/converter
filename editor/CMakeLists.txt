# 版本要求
cmake_minimum_required(VERSION 3.12.1)

SET( BuildType "editor")
#按照visual studio的目录结构分组
macro(source_group_by_dir source_files)
    if(MSVC)
        set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
        foreach(sgbd_file ${${source_files}})
            string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC)
endmacro(source_group_by_dir)

# 工程名
project(editor)

#第三方库路径
SET(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
SET(FBX_BIN ${THIRD_PARTY}/fbxsdk/lib/vs2017/x64)
SET(ENGINE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../engine)
SET(EDITOR_PATH ${CMAKE_CURRENT_SOURCE_DIR})
SET(FBXIMPORTER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../importer)
SET(FREEIMGE_PATH ${THIRD_PARTY}/freeimage)
SET(JSONCPP_PATH ${THIRD_PARTY}/jsoncpp)

#第三方库头文件
SET(THIRD_PARTY_INCLUDE ${THIRD_PARTY}/fbxsdk/include ${FREEIMGE_PATH}/include)

#add_subdirectory(${THIRD_PARTY}/jsoncpp)
add_subdirectory(${THIRD_PARTY}/imgui ${THIRD_PARTY}/imgui/${PROJECT_NAME}/build)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../engine ${CMAKE_CURRENT_SOURCE_DIR}/../engine/${PROJECT_NAME}/build)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../importer ${CMAKE_CURRENT_SOURCE_DIR}/../importer/${PROJECT_NAME}/build)
add_subdirectory(${THIRD_PARTY}/jsoncpp ${THIRD_PARTY}/jsoncpp/${PROJECT_NAME}/build)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine/third_party/jsoncpp)
#当前工程include目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include 
                    ${THIRD_PARTY_INCLUDE} 
                    ${ENGINE_PATH}/include 
                    ${JSONCPP_PATH}/include
                    ${FBXIMPORTER_PATH}/include
                    ${EDITOR_PATH}/include
                    )


# 添加编译宏
add_compile_definitions(FBXSDK_SHARED EDITOR PLATFORM_WINDOWS=1)

# 添加c++标准
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /GR")
else(MSVC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif(MSVC)

# cpp files
file(GLOB_RECURSE SRC_LIST "${EDITOR_PATH}/src/*.cpp" "${EDITOR_PATH}/src/*.h" )
# .h files
file(GLOB_RECURSE HEAD_LIST "${EDITOR_PATH}/include/*.h" )
# hlsl files
file(GLOB_RECURSE HLSL_LIST "${EDITOR_PATH}/src/*.hlsl")
# message(${HLSL_LIST})
# 添加到工程
add_executable(editor ${SRC_LIST} ${HEAD_LIST})

# 创建visual studio 目录
set(all_files ${HLSL_LIST} ${HEAD_LIST} ${SRC_LIST} )
source_group_by_dir(all_files)

#link target
target_link_libraries(editor $<$<CONFIG:Debug>:${THIRD_PARTY}/fbxsdk/lib/vs2017/x64/debug/libfbxsdk.lib> $<$<CONFIG:Release>:${THIRD_PARTY}/fbxsdk/lib/vs2017/x64/release/libfbxsdk.lib>)
target_link_libraries(editor $<$<CONFIG:Debug>:${FREEIMGE_PATH}/lib/FreeImaged.lib> $<$<CONFIG:Release>:${FREEIMGE_PATH}/lib/FreeImage.lib>)

# 支持windows窗口
set_target_properties(
		editor
		PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS /ignore:4099"
				   LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ignore:4099"
	)

# 设置debug目录
set_target_properties(editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug)

# 设置开始工程
set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT editor)

# 设置要链接的库
target_link_libraries(editor d3d11)
target_link_libraries(editor dxgi)
target_link_libraries(editor dxguid)	
target_link_libraries(editor d3dcompiler)
target_link_libraries(editor jsoncpp)
target_link_libraries(editor engine)
target_link_libraries(editor fbximporter)
target_link_libraries(editor imgui)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

# post build


add_custom_command(
		TARGET editor POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${FBX_BIN}/debug/libfbxsdk.dll> $<$<CONFIG:Release>:${FBX_BIN}/release/libfbxsdk.dll> 	$<TARGET_FILE_DIR:editor>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${FREEIMGE_PATH}/lib/FreeImaged.dll> $<$<CONFIG:Release>:${FREEIMGE_PATH}/lib/FreeImage.dll> 	$<TARGET_FILE_DIR:editor>
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../engine/src/Shader/ 	$<TARGET_FILE_DIR:editor>/Shader
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../resource/ 	$<TARGET_FILE_DIR:editor>/
        COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE_DIR:editor>/imgui.ini
)

#add_dependencies(editor jsoncpp solidangle imgui)
add_dependencies(editor engine jsoncpp fbximporter imgui)
