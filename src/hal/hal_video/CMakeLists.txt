project(camera_test)
cmake_minimum_required(VERSION 2.8.12)

#SET(CMAKE_C_COMPILER   arm-linux-gcc)
#SET(CMAKE_CXX_COMPILER arm-linux-g++)
#SET(CMAKE_CXX_FLAGS -std=c++11)

set(CT_SRC_FILES
    video.cpp
	camera/camerahal.cpp
	buffer/camerabuf.cpp
	process/display_process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lcd/lcd.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../out/system/include
)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../out/system/lib
)

add_definitions(
    -DLINUX -DSUPPORT_ION -DENABLE_ASSERT
)

add_executable(camera_test ${CT_SRC_FILES})


target_link_libraries(camera_test
    ion pthread adk cam_ia cam_engine_cifisp rkfb rkrga
)

target_include_directories(camera_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/camera
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buffer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/process
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lcd/
    PRIVATE ${MG_DISPLAY_INC}
)

install(TARGETS camera_test DESTINATION bin)
