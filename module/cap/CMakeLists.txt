
set(MODULE_VIDEO
    main.cpp
	camera/md_camerahal.cpp
	buffer/md_camerabuf.cpp
	process/md_display_process.cpp
)

add_definitions(
	-DLINUX -DSUPPORT_ION -DENABLE_ASSERT -DDEBUG
)

add_executable(cammer_cap ${MODULE_VIDEO})

target_link_libraries(cammer_cap
	-Wl,--start-group
    ion pthread cam_ia cam_engine_cifisp 
	drivers turbojpeg 
	-Wl,--end-group
)

