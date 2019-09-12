# 查找当前目录下的所有源文件 并将名称保存到 SRCS_MP4MUXERLIB 变量

set(SRCS_MP4MUXERLIB
	mp4_muxer.c
	)

# 生成链接库
add_library (mp4_muxer ${SRCS_MP4MUXERLIB})

target_link_libraries(mp4_muxer
	mp4v2
	)
