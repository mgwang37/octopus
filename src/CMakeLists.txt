SET(SRC_LIST main.cpp 
		       connection.cpp 
				 assembly_line.cpp
				 proxy_server.cpp
				 message_log.cpp
				 profile.cpp
				 dns_server.cpp
				 dns_cach.cpp
				 basic.cpp	
	)

ADD_EXECUTABLE(eversec-socks5 ${SRC_LIST})

TARGET_LINK_LIBRARIES(eversec-socks5 pthread)

