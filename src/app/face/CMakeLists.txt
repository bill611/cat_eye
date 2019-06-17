# 查找当前目录下的所有源文件 并将名称保存到 SRCS_FACE 变量
aux_source_directory(. SRCS_FACE)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_APP_FACE ${CMAKE_CURRENT_SOURCE_DIR})

set(SUB_DIRS_APP_FACE rdface)
#set(SUB_DIRS_APP_FACE rkface)
add_subdirectory(${SUB_DIRS_APP_FACE})


# 生成链接库
add_library (${CURRENT_FOLDER_APP_FACE} ${SRCS_FACE})
target_link_libraries (${CURRENT_FOLDER_APP_FACE} ${SUB_DIRS_APP_FACE})

