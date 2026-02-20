cmake_minimum_required(VERSION 3.20)
project(Master)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

add_definitions(-DSERVER_MASTER)

include(${CMAKE_CURRENT_LIST_DIR}/Source.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/FindMySQL.cmake)

set(MASTER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../Master")
set(MASTER_PLAYER "${MASTER_ROOT}/Player")
set(MASTER_SERVER "${MASTER_ROOT}/Server")

set(MASTER_FILES PRIVATE
    ${SOURCE_DATABASE}/Table/PlayerDBTable.cpp
    ${SOURCE_UTILS}/GameConfig.cpp

    ${SOURCE_PACKET}/NetPacket.cpp

    ${SOURCE_PLAYER}/Player.cpp
    ${SOURCE_PLAYER}/PlayerLoginDetail.cpp
    ${MASTER_PLAYER}/GamePlayer.cpp

    ${SOURCE_SERVER}/ServerBase.cpp
    ${SOURCE_SERVER}/ServerBroadwayBase.cpp
    ${MASTER_SERVER}/GameServer.cpp
    ${MASTER_SERVER}/ServerManager.cpp

    ${MASTER_ROOT}/Context.cpp
    ${MASTER_ROOT}/Main.cpp
)

add_executable(Master)
add_default_sources(Master)
target_sources(Master PRIVATE ${MASTER_FILES})

set_target_properties(Master PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../Runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../Runtime"
)

target_include_directories(Master PRIVATE
    ${SOURCE_ROOT}
    ${MASTER_ROOT}
    ${THIRD_PARTY_ENET}/include
    ${THIRD_PARTY_CONCURRENTQUEUE}/include
    ${MYSQL_INCLUDE_DIR}
)

target_link_libraries(Master PRIVATE
    ${MYSQL_LIB}
)

if(WIN32)
    target_link_libraries(Master PRIVATE
        winmm ws2_32
    )
endif(WIN32)