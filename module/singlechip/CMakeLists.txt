
add_executable(singlechip 
	singlechip.c
	)

target_link_libraries(singlechip
    pthread drivers
)

