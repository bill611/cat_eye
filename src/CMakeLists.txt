# 查找当前目录下的所有源文件
# 并将名称保存到 EXE_SRCS 变量
aux_source_directory(. EXE_SRCS)

# 增加子目录,增加此目录
set(SUB_DIRS 
	app 
	wireless
	gui
	hal
	drivers
	)

foreach(sub ${SUB_DIRS})
	add_subdirectory(${sub})
endforeach()

add_executable(${EXE} ${EXE_SRCS})

# 链接库
target_link_libraries (${EXE} ${SUB_DIRS})

