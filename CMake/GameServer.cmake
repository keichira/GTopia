cmake_minimum_required(VERSION 3.20)
project(GameServer)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

add_definitions(-DSERVER_GAME)

include(${CMAKE_CURRENT_LIST_DIR}/Source.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/FindMySQL.cmake)

set(GAME_ROOT "${CMAKE_CURRENT_LIST_DIR}/../GameServer")
set(GAME_PLAYER "${GAME_ROOT}/Player")
set(GAME_SERVER "${GAME_ROOT}/Server")

set(GAME_FILES PRIVATE
    ${SOURCE_DATABASE}/Table/PlayerDBTable.cpp
    ${SOURCE_UTILS}/GameConfig.cpp

    ${SOURCE_PACKET}/NetPacket.cpp

    ${SOURCE_PLAYER}/Player.cpp
    ${SOURCE_PLAYER}/PlayerLoginDetail.cpp
    #${GAME_PLAYER}/GamePlayer.cpp

    #${SOURCE_SERVER}/ServerBase.cpp
    ${SOURCE_SERVER}/ServerBroadwayBase.cpp
    #${GAME_SERVER}/GameServer.cpp
    ${GAME_SERVER}/MasterBroadway.cpp

    ${GAME_ROOT}/Context.cpp
    ${GAME_ROOT}/Main.cpp
)

add_executable(GameServer)
add_default_sources(GameServer)
target_sources(GameServer PRIVATE ${GAME_FILES})

set_target_properties(GameServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../Runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../Runtime"
)

target_include_directories(GameServer PRIVATE
    ${SOURCE_ROOT}
    ${GAME_ROOT}
    ${THIRD_PARTY_ENET}/include
    ${THIRD_PARTY_CONCURRENTQUEUE}/include
    ${MYSQL_INCLUDE_DIR}
)

target_link_libraries(GameServer PRIVATE
    ${MYSQL_LIB}
)

if(WIN32)
    target_link_libraries(GameServer PRIVATE
        winmm ws2_32
    )
endif(WIN32)