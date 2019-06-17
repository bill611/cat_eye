STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_APP_FACE_RK ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DUSE_MODEL_LOAD=0)
set(MODEL_LOAD_LIB model_loader)

add_definitions(-DUSE_SQLITE=1)
set(SQLITE_LIB sqlite3)

#if(DEFINED USE_DATABASE)
add_definitions(-DUSE_DATABASE=1)
set(DATABASE_SRC database/db_manager.cpp)
set(DATABASE_LIB database)
#endif(DEFINED USE_DATABASE)

add_definitions(-DUSE_FACE_CAPTURE=1)
set(FACE_CAPTURE_LIB rkmedia mpp)

add_definitions(-DUSE_H264_ENCODER=1)
set(H264_ENCODER_LIB rkmedia mpp)

set(SRC_HAL_FACE
    face.cpp
    face_service.cpp
    camera/cif_camera.cpp
    camera/isp_camera.cpp
    face/face_camera_buffer.cpp
    face/face_detector.cpp
    face/face_recognizer.cpp
    face/face_db_manager.cpp
    face/live_detector.cpp
    face/lib/rk_face_detect_library.cpp
    face/lib/rk_face_recognize_library.cpp
    face/lib/rk_live_detect_library.cpp
    process/capture_process.cpp
    process/display_process.cpp
    process/encoder.cpp
    system/display/display.cpp
    system/mm/ion_alloc.cpp
    system/time/time_utils.cpp
    ${DATABASE_SRC}
)

add_definitions(
	-DDEBUG
	-DLINUX -DSUPPORT_ION -DENABLE_ASSERT
    -DMAIN_APP_PRE_FACE_ROTATE=0 -DUSE_ROCKCHIP_FACE
	-DWIN_W=1280 -DWIN_H=720 -DVIDEO_DISPLAY_ROTATE_ANGLE=0
)

add_library (${CURRENT_FOLDER_APP_FACE_RK} ${SRC_HAL_FACE}) 

target_link_libraries(${CURRENT_FOLDER_APP_FACE_RK}
    rkrga rkfb ion pthread adk messenger nanomsg cjson
    cam_ia cam_engine_cifisp
    rk_face_detect rk_face_recognize
    ${MODEL_LOAD_LIB} ${SQLITE_LIB}
    ${DATABASE_LIB}
    ${USE_UVC_LIB}
    ${FACE_CAPTURE_LIB}
    ${H264_ENCODER_LIB}
	)
