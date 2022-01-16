# �汾Ҫ��
cmake_minimum_required(VERSION 3.12.1)

#����visual studio��Ŀ¼�ṹ����
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


SET(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

add_subdirectory(${THIRD_PARTY}/jsoncpp)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../libs)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${THIRD_PARTY}/jsoncpp/include )

# 添加c++标准
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /GR")
else(MSVC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif(MSVC)

# cpp files
file(GLOB_RECURSE SRC_LIST "src/*.cpp" "src/*.h" )
# .h files
file(GLOB_RECURSE HEAD_LIST "include/*.h" )
# hlsl files
file(GLOB_RECURSE HLSL_LIST "src/*.hlsl")

set(all_files ${HLSL_LIST} ${HEAD_LIST} ${SRC_LIST} )
source_group_by_dir(all_files)

add_library(engine STATIC ${SRC_LIST} ${HEAD_LIST})