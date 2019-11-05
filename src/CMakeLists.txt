SET(SRC_LIST main.cpp 
		       connection.cpp 
				 assembly_line.cpp
				 proxy_server.cpp
				 message_log.cpp
				 profile.cpp
				 dns_server.cpp
				 dns_cache.cpp
				 basic.cpp	
	)

ADD_EXECUTABLE(octopus-socks5 ${SRC_LIST})

TARGET_LINK_LIBRARIES(octopus-socks5 pthread)

