# 查找当前目录下的所有源文件
# 并将名称保存到 EXE_SRCS 变量
aux_source_directory(. EXE_SRCS)

# 增加子目录,增加此目录
set(SUB_DIRS 
	app 
	gui
	wireless
	drivers
	hal
	)

foreach(sub ${SUB_DIRS})
	add_subdirectory(${sub})
endforeach()

add_executable(${EXE} ${EXE_SRCS})


# 链接库
target_link_libraries (${EXE} 
	-Wl,--start-group
	${SUB_DIRS}
	sqlite3 qrencode paho-mqtt3a ssl curl cjson crypto
	minigui_ths png12 png jpeg freetype turbojpeg yuv 
	z m pthread dl rt asound
	ts rkrga rkfb ion stdc++
	-Wl,--end-group
	)

