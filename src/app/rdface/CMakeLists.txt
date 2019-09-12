# 查找当前目录下的所有源文件 并将名称保存到 SRCS_RDFACE 变量
aux_source_directory(. SRCS_RDFACE)

# 生成链接库
add_library (rdface ${SRCS_RDFACE})

target_link_libraries(rdface
	rsface ion rkrga pthread 
	)

