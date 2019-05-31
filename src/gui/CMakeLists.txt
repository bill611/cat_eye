# 查找当前目录下的所有源文件 并将名称保存到 SRCS_GUI 变量
aux_source_directory(. SRCS_GUI)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_GUI ${CMAKE_CURRENT_SOURCE_DIR})

set(SUB_DIRS_GUI 
	my_controls 
	)
foreach(sub ${SUB_DIRS_GUI})
	add_subdirectory(${sub})
endforeach()

# 生成链接库
add_library (${CURRENT_FOLDER_GUI} ${SRCS_GUI})
target_link_libraries (${CURRENT_FOLDER_GUI} ${SUB_DIRS_GUI})

