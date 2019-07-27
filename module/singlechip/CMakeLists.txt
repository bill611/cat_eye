aux_source_directory(. SRCS)

add_executable(singlechip ${SRCS})

target_link_libraries(singlechip
    pthread rt drivers hal
)

