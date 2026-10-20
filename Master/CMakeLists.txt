cmake_minimum_required(VERSION 3.20)
project(Master)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /DNDEBUG /O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /DNDEBUG /O2")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
else()
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
endif()

add_definitions(-DSERVER_MASTER)

include(${CMAKE_CURRENT_LIST_DIR}/../CMake/Source.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../CMake/FindMySQL.cmake)

set(MASTER_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(MASTER_PLAYER "${MASTER_ROOT}/Player")
set(MASTER_SERVER "${MASTER_ROOT}/Server")
set(MASTER_EVENT "${MASTER_ROOT}/Event")
set(MASTER_WORLD "${MASTER_ROOT}/World")
set(MASTER_COMMAND "${MASTER_ROOT}/Command")

set(MASTER_FILES PRIVATE
    ${SOURCE_DATABASE}/Table/PlayerDBTable.cpp
    ${SOURCE_DATABASE}/Table/WorldDBTable.cpp
    ${SOURCE_DATABASE}/Table/BanDBTable.cpp
    ${SOURCE_DATABASE}/Table/StatsDBTable.cpp
    ${SOURCE_DATABASE}/Table/HolidayDBTable.cpp
    ${SOURCE_UTILS}/GameConfig.cpp
    ${SOURCE_UTILS}/ConfigDB.cpp
    ${SOURCE_UTILS}/GrowUtils.cpp
    ${SOURCE_UTILS}/Base64.cpp

    ${SOURCE_HOLIDAY}/HolidayManager.cpp

    ${SOURCE_PACKET}/NetPacket.cpp
    ${SOURCE_PACKET}/PacketPool.cpp
    ${SOURCE_PACKET}/VariantPacket.cpp

    ${SOURCE_ITEM}/ItemInfo.cpp
    ${SOURCE_ITEM}/ItemInfoManager.cpp
    ${SOURCE_ITEM}/ItemUtils.cpp

    ${MASTER_EVENT}/TCP/TCPEvent_Player.cpp
    ${MASTER_EVENT}/TCP/TCPEvent_Server.cpp
    ${MASTER_EVENT}/TCP/TCPEvent_World.cpp
    ${MASTER_EVENT}/TCP/TCPEvent_Command.cpp

    ${SOURCE_WORLD}/WorldBalancer.cpp
    ${MASTER_WORLD}/WorldManager.cpp

    ${SOURCE_PLAYER}/Player.cpp
    ${SOURCE_PLAYER}/PlayerLoginDetail.cpp
    ${SOURCE_PLAYER}/RoleManager.cpp
    ${SOURCE_PLAYER}/Role.cpp
    ${SOURCE_PLAYER}/PlayModManager.cpp
    ${SOURCE_PLAYER}/PlayerPlayMod.cpp
    ${MASTER_PLAYER}/GamePlayer.cpp
    ${MASTER_PLAYER}/PlayerManager.cpp

    ${SOURCE_SERVER}/ServerBase.cpp
    ${SOURCE_SERVER}/ServerBroadwayBase.cpp
    ${SOURCE_SERVER}/StatsManager.cpp
    ${MASTER_SERVER}/GameServer.cpp
    ${MASTER_SERVER}/ServerManager.cpp
    ${MASTER_SERVER}/AdminServer.cpp
    ${MASTER_SERVER}/GamePresenceManager.cpp

    ${MASTER_COMMAND}/Commands.cpp

    ${SOURCE_NETWORK}/NetHTTP.cpp
    ${SOURCE_NETWORK}/DiscordWebhook.cpp

    ${MASTER_ROOT}/Context.cpp
    ${MASTER_ROOT}/Main.cpp
)

if(WIN32)
    list(APPEND MASTER_FILES ${SOURCE_NETWORK}/NetHTTP_WinHTTP.cpp)
else()
    if(NETHTTP_USE_CURL)
        find_package(CURL REQUIRED)
        list(APPEND MASTER_FILES ${SOURCE_NETWORK}/NetHTTP_Curl.cpp)
    else()
        list(APPEND MASTER_FILES ${SOURCE_NETWORK}/NetHTTP_Socket.cpp)
    endif()
endif()

add_executable(Master)
add_default_sources(Master)
add_database_sources(Master)
add_network_sources(Master)
target_sources(Master PRIVATE ${MASTER_FILES})

set_target_properties(Master PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Runtime"
)

target_include_directories(Master PRIVATE
    ${SOURCE_ROOT}
    ${MASTER_ROOT}
    ${MYSQL_INCLUDE_DIR}
)

if(NOT WIN32 AND NETHTTP_USE_CURL)
    target_include_directories(Master PRIVATE ${CURL_INCLUDE_DIRS})
    target_compile_definitions(Master PRIVATE NETHTTP_USE_CURL)
endif()

target_link_libraries(Master PRIVATE
    ${MYSQL_LIB}
)

if(WIN32)
    target_link_libraries(Master PRIVATE
        winmm ws2_32 winhttp
    )
    target_compile_definitions(Master PRIVATE WIN32_LEAN_AND_MEAN)
else()
    if(NETHTTP_USE_CURL)
        target_link_libraries(GameServer PRIVATE ${CURL_LIBRARIES})
    endif()
endif()

copy_dll(Master "${MYSQL_LIB_DIR}/libmysql.dll")

file(GLOB OPENSSL_DLLS
    "${MYSQL_LIB_DIR}/../bin/libcrypto*.dll"
    "${MYSQL_LIB_DIR}/../bin/libssl*.dll"
)
foreach(DLL ${OPENSSL_DLLS})
    copy_dll(Master "${DLL}")
endforeach()