# 查找当前目录下的所有源文件 并将名称保存到 SRCS_UCPAAS 变量
aux_source_directory(. SRCS_UCPAAS)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_UCPAAS ${CMAKE_CURRENT_SOURCE_DIR})

# 生成链接库
add_library (${CURRENT_FOLDER_UCPAAS} ${SRCS_UCPAAS})
target_link_libraries(${CURRENT_FOLDER_UCPAAS}
	UCSService UcsEngine c
	)

