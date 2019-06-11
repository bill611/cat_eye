# 查找当前目录下的所有源文件 并将名称保存到 SRCS_APP 变量
aux_source_directory(. SRCS_APP)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_APP ${CMAKE_CURRENT_SOURCE_DIR})

if (MODE_UCPASS)
	add_subdirectory(ucpaas)
endif()
# 生成链接库
add_library (${CURRENT_FOLDER_APP} ${SRCS_APP})
if (MODE_UCPASS)
	target_link_libraries (${CURRENT_FOLDER_APP} ucpaas)
endif()

