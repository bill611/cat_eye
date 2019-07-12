STRING( REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER_APP_UDPTALK ${CMAKE_CURRENT_SOURCE_DIR})


set(SRC_APP_UDPTALK
    udp_talk_protocol.c
	udp_talk_transport.c
	udp_talk_parse.c
	udp_client.c
	share_memory.c
)


add_library (${CURRENT_FOLDER_APP_UDPTALK} ${SRC_APP_UDPTALK}) 



