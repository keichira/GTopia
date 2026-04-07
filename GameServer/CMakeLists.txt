cmake_minimum_required(VERSION 3.20)
project(GameServer)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-D_NDEBUG -O2)
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
set(GAME_DIALOG "${GAME_PLAYER}/Dialog")

set(GAME_FILES PRIVATE
    ${SOURCE_DATABASE}/Table/PlayerDBTable.cpp
    ${SOURCE_DATABASE}/Table/WorldDBTable.cpp
    ${SOURCE_UTILS}/GameConfig.cpp
    ${SOURCE_UTILS}/DialogBuilder.cpp

    ${SOURCE_PACKET}/NetPacket.cpp

    ${SOURCE_ITEM}/ItemInfo.cpp
    ${SOURCE_ITEM}/ItemInfoManager.cpp
    ${SOURCE_ITEM}/ItemUtils.cpp

    ${GAME_EVENT}/TCP/TCPEventHello.cpp
    ${GAME_EVENT}/TCP/TCPEventAuth.cpp
    ${GAME_EVENT}/TCP/TCPEventPlayerSession.cpp
    ${GAME_EVENT}/TCP/TCPEventWorldInit.cpp
    ${GAME_EVENT}/TCP/TCPEventRenderWorld.cpp
    ${GAME_EVENT}/TCP/TCPEventWorldSendPlayer.cpp
    ${GAME_EVENT}/TCP/TCPEventKillServer.cpp

    ${GAME_EVENT}/UDP/GameMessage/RefreshItemData.cpp
    ${GAME_EVENT}/UDP/GameMessage/EnterGame.cpp
    ${GAME_EVENT}/UDP/GameMessage/RefreshTributeData.cpp
    ${GAME_EVENT}/UDP/GameMessage/JoinRequest.cpp
    ${GAME_EVENT}/UDP/GameMessage/Input.cpp
    ${GAME_EVENT}/UDP/GameMessage/QuitToExit.cpp
    ${GAME_EVENT}/UDP/GameMessage/DialogReturn.cpp
    ${GAME_EVENT}/UDP/GameMessage/Trash.cpp

    ${GAME_EVENT}/UDP/GamePacket/ItemActivateRequest.cpp
    ${GAME_EVENT}/UDP/GamePacket/TileChangeRequest.cpp
    ${GAME_EVENT}/UDP/GamePacket/State.cpp

    ${GAME_COMMAND}/RenderWorld.cpp
    ${GAME_COMMAND}/GiveItem.cpp
    ${GAME_COMMAND}/Ghost.cpp
    ${GAME_COMMAND}/TogglePlayMod.cpp
    ${GAME_COMMAND}/Magic.cpp

    ${SOURCE_WORLD}/TileExtra.cpp
    ${SOURCE_WORLD}/TileInfo.cpp
    ${SOURCE_WORLD}/WorldInfo.cpp
    ${SOURCE_WORLD}/WorldTileManager.cpp
    ${SOURCE_WORLD}/WorldObjectManager.cpp
    ${GAME_WORLD}/WorldManager.cpp
    ${GAME_WORLD}/World.cpp

    ${GAME_DIALOG}/PlayerDialog.cpp
    ${GAME_DIALOG}/SignDialog.cpp
    ${GAME_DIALOG}/TrashDialog.cpp
    ${GAME_DIALOG}/LockDialog.cpp
    ${GAME_DIALOG}/RenderWorldDialog.cpp

    ${SOURCE_PLAYER}/Role.cpp
    ${SOURCE_PLAYER}/RoleManager.cpp
    ${SOURCE_PLAYER}/Player.cpp
    ${SOURCE_PLAYER}/PlayerTribute.cpp
    ${SOURCE_PLAYER}/PlayerInventory.cpp
    ${SOURCE_PLAYER}/PlayerLoginDetail.cpp
    ${SOURCE_PLAYER}/CharacterData.cpp
    ${SOURCE_PLAYER}/PlayMod.cpp
    ${SOURCE_PLAYER}/PlayModManager.cpp
    ${GAME_PLAYER}/GamePlayer.cpp

    ${SOURCE_SERVER}/ServerBase.cpp
    ${SOURCE_SERVER}/ServerBroadwayBase.cpp
    ${GAME_SERVER}/GameServer.cpp
    ${GAME_SERVER}/MasterBroadway.cpp

    ${GAME_ROOT}/Context.cpp
    ${GAME_ROOT}/Main.cpp
)

add_executable(GameServer)
add_default_sources(GameServer)
add_database_sources(GameServer)
add_network_sources(GameServer)
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

target_link_libraries(GameServer PRIVATE
    ${MYSQL_LIB}
)

if(WIN32)
    target_link_libraries(GameServer PRIVATE
        winmm ws2_32
    )
endif(WIN32)