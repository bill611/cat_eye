# 查找当前目录下的所有源文件 并将名称保存到 SRCS_AVILIB 变量
aux_source_directory(. SRCS_AVILIB)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_AVILIB ${CMAKE_CURRENT_SOURCE_DIR})

# 生成链接库
add_library (${CURRENT_FOLDER_AVILIB} ${SRCS_AVILIB})

