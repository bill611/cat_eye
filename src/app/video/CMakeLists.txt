STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_APP_VIDEO ${CMAKE_CURRENT_SOURCE_DIR})


set(SRC_APP_VIDEO
    video_server.cpp
	h264_enc_dec/mpi_dec_api.c
	h264_enc_dec/mpi_enc_api.c
	h264_enc_dec/h264_split.c
	camera/camerahal.cpp
	buffer/camerabuf.cpp
	process/display_process.cpp
	process/face_process.cpp
	process/encoder_process.cpp
)


add_definitions(
	-DLINUX -DSUPPORT_ION -DENABLE_ASSERT -DDEBUG
)

add_library (${CURRENT_FOLDER_APP_VIDEO} ${SRC_APP_VIDEO}) 

target_link_libraries(${CURRENT_FOLDER_APP_VIDEO}
    ion pthread adk cam_ia cam_engine_cifisp rkfb rkrga
	yuv
	rkmedia mpp
)

