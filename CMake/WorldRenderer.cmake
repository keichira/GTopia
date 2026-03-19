cmake_minimum_required(VERSION 3.20)
project(WorldRenderer)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-D_NDEBUG -O2)
endif()

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DUSE_ZLIB)

include(${CMAKE_CURRENT_LIST_DIR}/Source.cmake)
set(WORLD_RENDERER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../WorldRenderer")
set(WORLD_RENDERER_EVENT "${CMAKE_CURRENT_LIST_DIR}/../WorldRenderer/Event")

set(BLEND2D_STATIC TRUE)
include(${THIRD_PARTY_BLEND2D}/CMakeLists.txt)

set(WORLD_RENDERER_FILES PRIVATE
    ${SOURCE_ITEM}/ItemInfo.cpp
    ${SOURCE_ITEM}/ItemInfoManager.cpp
    ${SOURCE_ITEM}/ItemUtils.cpp

    ${SOURCE_NETWORK}/NetClient.cpp
    ${SOURCE_NETWORK}/NetSocket.cpp

    ${SOURCE_WORLD}/TileExtra.cpp
    ${SOURCE_WORLD}/TileInfo.cpp
    ${SOURCE_WORLD}/WorldInfo.cpp
    ${SOURCE_WORLD}/WorldTileManager.cpp
    ${SOURCE_WORLD}/WorldObjectManager.cpp
    ${WORLD_RENDERER_ROOT}/WorldRenderer.cpp

    ${SOURCE_UTILS}/ResourceManager.cpp
    ${SOURCE_UTILS}/GameConfig.cpp
    ${SOURCE_PROTON}/ProtonUtils.cpp

    ${WORLD_RENDERER_ROOT}/MasterBroadway.cpp
    ${SOURCE_SERVER}/ServerBroadwayBase.cpp

    ${WORLD_RENDERER_EVENT}/TCPEventAuth.cpp
    ${WORLD_RENDERER_EVENT}/TCPEventHello.cpp
    ${WORLD_RENDERER_EVENT}/TCPEventRenderWorld.cpp

    ${WORLD_RENDERER_ROOT}/Renderer2D.cpp
    ${WORLD_RENDERER_ROOT}/WeatherRenderer.cpp
    ${WORLD_RENDERER_ROOT}/WorldRendererManager.cpp

    ${WORLD_RENDERER_ROOT}/Context.cpp
    ${WORLD_RENDERER_ROOT}/Main.cpp
)

add_executable(WorldRenderer)
add_default_sources(WorldRenderer)
add_zlib_sources(WorldRenderer)
target_sources(WorldRenderer PRIVATE ${WORLD_RENDERER_FILES})

set_target_properties(WorldRenderer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../Runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../Runtime"
)

target_include_directories(WorldRenderer PRIVATE
    ${SOURCE_ROOT}
    ${WORLD_RENDERER_ROOT}
    ${THIRD_PARTY_BLEND2D}
)

target_link_libraries(WorldRenderer PRIVATE
    blend2d::blend2d
)