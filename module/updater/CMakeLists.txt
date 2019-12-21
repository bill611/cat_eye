add_definitions(-DUSE_NOR)

set(UPDATER_SRC_FILES 
	updater.cpp
	verify/crc/crc32.cpp
	verify/md5/md5sum.cpp
	display/display.cpp
	wget/httpd.cpp
	partition/partition.cpp
)

add_executable(updater ${UPDATER_SRC_FILES})


target_include_directories(updater
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/verify/crc
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/verify/md5
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/display
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wget
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/partition
)

