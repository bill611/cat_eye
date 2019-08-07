
set(MODULE_VIDEO
    main.cpp
	h264_enc_dec/md_mpi_enc_api.c
	camera/md_camerahal.cpp
	buffer/md_camerabuf.cpp
	process/md_display_process.cpp
	process/md_encoder_process.cpp
)

add_definitions(
	-DLINUX -DSUPPORT_ION -DENABLE_ASSERT -DDEBUG
)

add_executable(cammer_cap ${MODULE_VIDEO})

target_link_libraries(cammer_cap
	-Wl,--start-group
    ion pthread adk cam_ia cam_engine_cifisp 
	rkmedia mpp drivers
	turbojpeg 
	yuv rt
	-Wl,--end-group
)

