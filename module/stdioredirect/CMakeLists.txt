aux_source_directory(. SRCS)

add_definitions(
	-DLINUX -DSUPPORT_ION -DENABLE_ASSERT -DDEBUG
)

add_executable(red ${SRCS})

target_link_libraries(red
    pthread 
)

