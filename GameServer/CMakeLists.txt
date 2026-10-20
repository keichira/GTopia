cmake_minimum_required(VERSION 3.20)
project(GameServer)

include(ExternalProject)

set(CMAKE_CXX_STANDARD 17)

# Needed for HTTPS requests
option(NETHTTP_USE_CURL "Use libcurl for HTTP requests on non Windows" OFF)

# Lua configs
option(ENABLE_LUA_SCRIPTING "Enable Lua scripting support" OFF)

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

add_definitions(-DSERVER_GAME)

include(${CMAKE_CURRENT_LIST_DIR}/../CMake/Source.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../CMake/FindMySQL.cmake)

set(GAME_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(GAME_PLAYER "${GAME_ROOT}/Player")
set(GAME_SERVER "${GAME_ROOT}/Server")
set(GAME_EVENT "${GAME_ROOT}/Event")
set(GAME_WORLD "${GAME_ROOT}/World")
set(GAME_COMMAND "${GAME_ROOT}/Command")
set(GAME_DIALOG "${GAME_ROOT}/Dialog")
set(GAME_STORE "${GAME_ROOT}/Store")
set(GAME_ITEM "${GAME_ROOT}/Item")
set(GAME_LUA "${GAME_ROOT}/Lua")

if(ENABLE_LUA_SCRIPTING)
    if(NOT EXISTS "${THIRD_PARTY_LUAJIT}/src/lua.h")
        message(FATAL_ERROR "LuaJIT submodule not found or uninitialized!\nPlease run: git submodule update --init --recursive ThirdParty/luajit")
    endif()
endif()

set(GAME_FILES PRIVATE
    ${SOURCE_DATABASE}/Table/PlayerDBTable.cpp
    ${SOURCE_DATABASE}/Table/WorldDBTable.cpp
    ${SOURCE_DATABASE}/Table/BanDBTable.cpp
    ${SOURCE_DATABASE}/Table/StatsDBTable.cpp
    ${SOURCE_DATABASE}/Table/HolidayDBTable.cpp
    ${SOURCE_UTILS}/GameConfig.cpp
    ${SOURCE_UTILS}/DialogBuilder.cpp
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

    ${GAME_EVENT}/TCP/TCPEvent_Command.cpp
    ${GAME_EVENT}/TCP/TCPEvent_Player.cpp
    ${GAME_EVENT}/TCP/TCPEvent_Server.cpp
    ${GAME_EVENT}/TCP/TCPEvent_World.cpp

    ${GAME_EVENT}/UDP/GameMessage/GameMessage_Input.cpp
    ${GAME_EVENT}/UDP/GameMessage/GameMessage_Item.cpp
    ${GAME_EVENT}/UDP/GameMessage/GameMessage_Player.cpp

    ${GAME_EVENT}/UDP/GamePacket/ItemActivateRequest.cpp
    ${GAME_EVENT}/UDP/GamePacket/TileChangeRequest.cpp
    ${GAME_EVENT}/UDP/GamePacket/State.cpp
    ${GAME_EVENT}/UDP/GamePacket/ObjectActivateRequest.cpp
    ${GAME_EVENT}/UDP/GamePacket/TileActivateRequest.cpp

    ${GAME_COMMAND}/Commands.cpp

    ${SOURCE_WORLD}/TileExtra.cpp
    ${SOURCE_WORLD}/TileInfo.cpp
    ${SOURCE_WORLD}/WorldInfo.cpp
    ${SOURCE_WORLD}/WorldTileManager.cpp
    ${SOURCE_WORLD}/WorldObjectManager.cpp
    ${SOURCE_WORLD}/WorldBalancer.cpp
    ${GAME_WORLD}/WorldManager.cpp
    ${GAME_WORLD}/World.cpp
    ${GAME_WORLD}/WorldNPC.cpp
    ${GAME_WORLD}/WorldNPCManager.cpp
    ${GAME_WORLD}/WorldBossManager.cpp
    ${GAME_WORLD}/WorldEventManager.cpp

    ${GAME_DIALOG}/GameDialogs.cpp
    ${GAME_DIALOG}/DialogManager.cpp

    ${SOURCE_PLAYER}/Role.cpp
    ${SOURCE_PLAYER}/RoleManager.cpp
    ${SOURCE_PLAYER}/Player.cpp
    ${SOURCE_PLAYER}/PlayerTribute.cpp
    ${SOURCE_PLAYER}/PlayerInventory.cpp
    ${SOURCE_PLAYER}/PlayerLoginDetail.cpp
    ${SOURCE_PLAYER}/PlayModManager.cpp
    ${SOURCE_PLAYER}/PlayerPlayMod.cpp
    ${GAME_PLAYER}/GamePlayer.cpp
    ${GAME_PLAYER}/PlayerManager.cpp
    ${GAME_PLAYER}/AchievementManager.cpp
    ${GAME_PLAYER}/PlayerProgress.cpp
    ${GAME_PLAYER}/PlayerPlayModController.cpp
    ${GAME_PLAYER}/PlayerTrade.cpp
    ${GAME_PLAYER}/PlayerExtraData.cpp

    ${GAME_STORE}/StoreManager.cpp
    ${GAME_ITEM}/HarmonicCrystal.cpp
    ${GAME_ITEM}/SuckerBlockManager.cpp

    ${SOURCE_NETWORK}/IPTracker.cpp

    ${SOURCE_SERVER}/ServerBase.cpp
    ${SOURCE_SERVER}/ServerBroadwayBase.cpp
    ${SOURCE_SERVER}/StatsManager.cpp
    ${GAME_SERVER}/GameServer.cpp
    ${GAME_SERVER}/MasterBroadway.cpp
    ${GAME_SERVER}/UserCacheManager.cpp
    ${GAME_SERVER}/GamePresenceManager.cpp

    ${SOURCE_NETWORK}/NetHTTP.cpp
    ${SOURCE_NETWORK}/DiscordWebhook.cpp

    ${GAME_ROOT}/Context.cpp
    ${GAME_ROOT}/Main.cpp
)

if(ENABLE_LUA_SCRIPTING)
    list(APPEND GAME_FILES 
        ${GAME_LUA}/LuaManager.cpp 
        ${GAME_LUA}/Bindings/PlayerBinding.cpp
        ${GAME_LUA}/Bindings/WorldBinding.cpp
        ${GAME_LUA}/Bindings/ItemBinding.cpp
        ${GAME_LUA}/Bindings/CommandBinding.cpp
        ${GAME_LUA}/Bindings/DialogBinding.cpp
        ${GAME_LUA}/Bindings/MathBinding.cpp
        ${GAME_LUA}/Bindings/DiscordBinding.cpp
    )
endif()

if(WIN32)
    list(APPEND GAME_FILES ${SOURCE_NETWORK}/NetHTTP_WinHTTP.cpp)
else()
    if(NETHTTP_USE_CURL)
        find_package(CURL REQUIRED)
        list(APPEND GAME_FILES ${SOURCE_NETWORK}/NetHTTP_Curl.cpp)
    else()
        list(APPEND GAME_FILES ${SOURCE_NETWORK}/NetHTTP_Socket.cpp)
    endif()
endif()

add_executable(GameServer)
add_default_sources(GameServer)
add_database_sources(GameServer)
add_network_sources(GameServer)
add_zlib_sources(GameServer)
target_sources(GameServer PRIVATE ${GAME_FILES})

set_target_properties(GameServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Runtime"
)

target_include_directories(GameServer PRIVATE
    ${SOURCE_ROOT}
    ${GAME_ROOT}
    ${MYSQL_INCLUDE_DIR}
)

if(ENABLE_LUA_SCRIPTING)
    target_compile_definitions(GameServer PRIVATE ENABLE_LUA_SCRIPTING)

    if(MSVC)
        ExternalProject_Add(
            luajit_build
            SOURCE_DIR "${THIRD_PARTY_LUAJIT}"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND cmd /c "cd /d ${THIRD_PARTY_LUAJIT}\\src && msvcbuild.bat static"
            INSTALL_COMMAND ""
            BUILD_BYPRODUCTS "${THIRD_PARTY_LUAJIT}/src/lua51.lib"
        )
        set(LUAJIT_LIB "${THIRD_PARTY_LUAJIT}/src/lua51.lib")
    else()
        ExternalProject_Add(
            luajit_build
            SOURCE_DIR "${THIRD_PARTY_LUAJIT}"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND make -C "${THIRD_PARTY_LUAJIT}/src" amalg
            INSTALL_COMMAND ""
            BUILD_BYPRODUCTS "${THIRD_PARTY_LUAJIT}/src/libluajit.a"
        )
        set(LUAJIT_LIB "${THIRD_PARTY_LUAJIT}/src/libluajit.a")
    endif()

    add_dependencies(GameServer luajit_build)
    target_include_directories(GameServer PRIVATE "${THIRD_PARTY_LUAJIT}/src")
    target_link_libraries(GameServer PRIVATE "${LUAJIT_LIB}")
endif()

if(NOT WIN32 AND NETHTTP_USE_CURL)
    target_include_directories(GameServer PRIVATE ${CURL_INCLUDE_DIRS})
    target_compile_definitions(GameServer PRIVATE NETHTTP_USE_CURL)
endif()

target_link_libraries(GameServer PRIVATE
    ${MYSQL_LIB}
)

if(WIN32)
    target_link_libraries(GameServer PRIVATE
        winmm ws2_32 winhttp
    )
    target_compile_definitions(GameServer PRIVATE WIN32_LEAN_AND_MEAN)
else()
    if(NETHTTP_USE_CURL)
        target_link_libraries(GameServer PRIVATE ${CURL_LIBRARIES})
    endif()
endif()

if(WIN32)
    copy_dll(GameServer "${MYSQL_LIB_DIR}/libmysql.dll")

    file(GLOB OPENSSL_DLLS
        "${MYSQL_LIB_DIR}/../bin/libcrypto*.dll"
        "${MYSQL_LIB_DIR}/../bin/libssl*.dll"
    )
    foreach(DLL ${OPENSSL_DLLS})
        copy_dll(GameServer "${DLL}")
    endforeach()
endif()