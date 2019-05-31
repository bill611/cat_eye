# 查找当前目录下的所有源文件 并将名称保存到 SRCS_WIRELESS 变量
aux_source_directory(. SRCS_WIRELESS)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_WIRELESS ${CMAKE_CURRENT_SOURCE_DIR})

# 生成链接库
add_library (${CURRENT_FOLDER_WIRELESS} ${SRCS_WIRELESS})

