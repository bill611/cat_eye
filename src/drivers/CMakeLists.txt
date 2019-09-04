# 查找当前目录下的所有源文件 并将名称保存到 SRCS_DRIVERS 变量
aux_source_directory(. SRCS_DRIVERS)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_DRIVERS ${CMAKE_CURRENT_SOURCE_DIR})

set(SUB_DIRS_DRIVERS 
	iniparser 
	avilib 
	)
foreach(sub ${SUB_DIRS_DRIVERS})
	add_subdirectory(${sub})
endforeach()


# 生成链接库
add_library (${CURRENT_FOLDER_DRIVERS} ${SRCS_DRIVERS})
target_link_libraries (${CURRENT_FOLDER_DRIVERS} ${SUB_DIRS_DRIVERS})

