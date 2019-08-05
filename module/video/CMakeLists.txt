
set(MODULE_VIDEO
    main.cpp
	h264_enc_dec/mpi_enc_api.c
	camera/camerahal.cpp
	buffer/camerabuf.cpp
	process/display_process.cpp
	process/encoder_process.cpp
)

add_definitions(
	-DLINUX -DSUPPORT_ION -DENABLE_ASSERT -DDEBUG
)

add_executable(cammer_video ${MODULE_VIDEO})

target_link_libraries(cammer_video
	-Wl,--start-group
    ion pthread adk cam_ia cam_engine_cifisp rkfb rkrga 
	rkmedia mpp drivers
	turbojpeg 
	yuv rt
	-Wl,--end-group
)

