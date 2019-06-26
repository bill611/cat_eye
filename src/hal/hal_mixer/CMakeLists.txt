# 查找当前目录下的所有源文件 并将名称保存到 SRCS_HAL_MIXER 变量
aux_source_directory(. SRCS_HAL_MIXER)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_HAL_MIXER ${CMAKE_CURRENT_SOURCE_DIR})

# 生成链接库
add_library (${CURRENT_FOLDER_HAL_MIXER} ${SRCS_HAL_MIXER})

