# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目名称
project (cmake_project_mode)

# 项目版本
set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
configure_file (
    "${PROJECT_SOURCE_DIR}/include/cmake_config.h.in"
    "${PROJECT_SOURCE_DIR}/include/cmake_config.h"
    )

# 设置默认编译平台
option(PLATFORM_RV1108 "This is a option for rv1108" ON)
# 设置编译功能
option(MODE_UCPAAS "This is a option for ucpass" ON)
option(MODE_FACE "This is a option for face" ON)
option(MODE_UART "This is a option for uart" ON)
option(MODE_VIDEO "This is a option for video" ON)
option(MODE_UDPTALK "This is a option for udptalk" ON)

# 添加编译过程变量
# set(CMAKE_VERBOSE_MAKEFILE ON)

# 设置交叉编译环境 ------------
set(CMAKE_SYSTEM_NAME Linux)

if (PLATFORM_RV1108)
	set(TOOLCHAIN_DIR $ENV{RV1108_CROOS_PATH})
	set(TOOLCHAIN  ${TOOLCHAIN_DIR}/bin/arm-linux-)
endif()

set(CMAKE_C_COMPILER   	${TOOLCHAIN}gcc)
set(CMAKE_CXX_COMPILER 	${TOOLCHAIN}g++)
set(CMAKE_AR   			${TOOLCHAIN}ar)
set(CMAKE_LINKER   		${TOOLCHAIN}ld)
set(CMAKE_NM   			${TOOLCHAIN}nm)
set(CMAKE_OBJDUMP   	${TOOLCHAIN}objdump)
set(CMAKE_RANLIB   		${TOOLCHAIN}ranlib)
set(CMAKE_STRIP   		${TOOLCHAIN}strip)

if (PLATFORM_RV1108)
	set(CMAKE_C_FLAGS "-DRV1108 ${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "-DRV1108 ${CMAKE_CXX_FLAGS}")
else()
	set(CMAKE_C_FLAGS "-DX86 ${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "-DX86 ${CMAKE_CXX_FLAGS}")
endif()

# 设置debug编译和release编译配置
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DWATCHDOG_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DWATCHDOG_DEBUG")

set(CMAKE_C_FLAGS_RELEASE "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

# 设置交叉编译环境 ------------

#设置执行文件存放路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

#设置库文件存放路径
#set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/libs)

#功能函数获取当前目录及子目录(递归获取),添加到头文件搜索路径
function(func_include_sub_directories_recursively root_dir)
    if (IS_DIRECTORY ${root_dir})               # 当前路径如果是一个目录,则加入到包含目录
        include_directories(${root_dir})
    endif()

    file(GLOB ALL_SUB RELATIVE ${root_dir} ${root_dir}/*) # 获得当前目录下的所有文件，加如ALL_SUB列表中

    foreach(sub ${ALL_SUB})
        if (IS_DIRECTORY ${root_dir}/${sub})
            func_include_sub_directories_recursively(${root_dir}/${sub}) # 对子目录递归调用，包含
        endif()
    endforeach()
endfunction()


# 项目的所有目录都为头文件搜索路径
func_include_sub_directories_recursively(${PROJECT_SOURCE_DIR}/src)
func_include_sub_directories_recursively(${PROJECT_SOURCE_DIR}/module)
#func_include_sub_directories_recursively(${PROJECT_SOURCE_DIR}/include)

# 增加其他目录头文件路径
include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include/mqtt
	)
if (PLATFORM_RV1108)
	include_directories(
		${PROJECT_SOURCE_DIR}/include/sdk_include
		)
endif()

# 添加库文件搜索路径
if (PLATFORM_RV1108)
	link_directories(
		${PROJECT_SOURCE_DIR}/lib/arm
		${PROJECT_SOURCE_DIR}/lib/arm/sdk_system_lib
		${PROJECT_SOURCE_DIR}/lib/arm/sdk_rootfs_lib
		${PROJECT_SOURCE_DIR}/lib/arm/sdk_ucpaas_lib
		)
else()
	link_directories(
		${PROJECT_SOURCE_DIR}/lib/x86
		/usr/local/lib
		/usr/lib/x86_64-linux-gnu
		/usr/lib
		)

endif()

# 添加库文件名称
link_libraries (
	pthread asound 
	)

if (MODE_UCPAAS)
	add_definitions(-DUSE_UCPAAS)
endif()
if (MODE_FACE)
	add_definitions(-DUSE_FACE)
endif()
if (MODE_UART)
	add_definitions(-DUSE_UART)
endif()
if (MODE_VIDEO)
    add_definitions(-DUSE_VIDEO)
endif()
if (MODE_UDPTALK)
	add_definitions(-DUSE_UDPTALK)
endif()

# 添加宏定义
# include(define.cmake)

# 增加打印信息
# message(STATUS "message test ${PROJECT_SOURCE_DIR}")

# 指定应用名称
set(EXE cat_eye)

# 增加子目录,可以传递参数
add_subdirectory(src)
add_subdirectory(module)

