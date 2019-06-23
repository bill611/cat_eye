# 查找当前目录下的所有源文件 并将名称保存到 SRCS_APP 变量
aux_source_directory(. SRCS_APP)

STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_APP ${CMAKE_CURRENT_SOURCE_DIR})


if (MODE_UCPAAS)
	set(SUB_DIRS_APP ${SUB_DIRS_APP} ucpaas )
endif()

if (MODE_VIDEO)
	set(SUB_DIRS_APP ${SUB_DIRS_APP} video )
endif()

if (MODE_FACE)
	set(SUB_DIRS_APP ${SUB_DIRS_APP} face )
endif()

foreach(sub ${SUB_DIRS_APP})
	add_subdirectory(${sub})
endforeach()

# 生成链接库
add_library (${CURRENT_FOLDER_APP} ${SRCS_APP})
target_link_libraries (${CURRENT_FOLDER_APP} ${SUB_DIRS_APP})

