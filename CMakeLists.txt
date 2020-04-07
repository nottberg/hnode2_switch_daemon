CMAKE_MINIMUM_REQUIRED( VERSION 2.8 ) 
PROJECT( hnode_switchd ) 

#SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )

SET( HNSWITCHD_SRC
     ${CMAKE_SOURCE_DIR}/src/daemon/hnswitchd.cpp
     ${CMAKE_SOURCE_DIR}/src/daemon/HNSwitchDaemon.cpp
     ${CMAKE_SOURCE_DIR}/src/daemon/HNDaemonLog.cpp
     ${CMAKE_SOURCE_DIR}/src/daemon/HNScheduleMatrix.cpp
     ${CMAKE_SOURCE_DIR}/src/daemon/HNSwitchManager.cpp
     ${CMAKE_SOURCE_DIR}/src/daemon/HNSWDPacketDaemon.cpp
)

SET( HNSWITCHD_CLIENT_SRC
     ${CMAKE_SOURCE_DIR}/src/client/hnswitchd_client.cpp
     ${CMAKE_SOURCE_DIR}/src/client/HNSWDPacketClient.cpp
)

SET(CMAKE_BUILD_TYPE Debug)

FIND_PACKAGE( Poco REQUIRED Foundation Net NetSSL JSON )

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/include )

ADD_EXECUTABLE( hnswitchd ${HNSWITCHD_SRC} )
TARGET_LINK_LIBRARIES( hnswitchd PRIVATE Poco::Foundation Poco::Net Poco::NetSSL Poco::JSON )
TARGET_LINK_LIBRARIES( hnswitchd PRIVATE "-li2c" )

ADD_EXECUTABLE( hnswitchd_client ${HNSWITCHD_CLIENT_SRC} )
TARGET_LINK_LIBRARIES( hnswitchd_client PRIVATE Poco::Foundation Poco::Net Poco::NetSSL Poco::JSON )

