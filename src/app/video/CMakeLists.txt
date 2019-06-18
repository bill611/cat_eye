STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_APP_VIDEO ${CMAKE_CURRENT_SOURCE_DIR})


set(SRC_APP_VIDEO
    video.cpp
	camera/camerahal.cpp
	buffer/camerabuf.cpp
	process/display_process.cpp
    lcd/lcd.c
)


add_definitions(
    -DLINUX -DSUPPORT_ION -DENABLE_ASSERT
)

add_library (${CURRENT_FOLDER_APP_VIDEO} ${SRC_APP_VIDEO}) 

target_link_libraries(${CURRENT_FOLDER_APP_VIDEO}
    ion pthread adk cam_ia cam_engine_cifisp rkfb rkrga
)

