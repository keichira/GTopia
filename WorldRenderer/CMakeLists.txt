cmake_minimum_required(VERSION 3.20)
project(WorldRenderer)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /D_NDEBUG /O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /D_NDEBUG /O2")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
else()
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -D_NDEBUG -O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_NDEBUG -O2")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../CMake/Source.cmake)
set(WORLD_RENDERER_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(WORLD_RENDERER_EVENT "${WORLD_RENDERER_ROOT}/Event")

set(BLEND2D_STATIC TRUE)
set(ASMJIT_DIR ${THIRD_PARTY_ASMJIT})
include(${THIRD_PARTY_BLEND2D}/CMakeLists.txt)

set(WORLD_RENDERER_FILES PRIVATE
    ${SOURCE_ITEM}/ItemInfo.cpp
    ${SOURCE_ITEM}/ItemInfoManager.cpp
    ${SOURCE_ITEM}/ItemUtils.cpp
    ${SOURCE_ITEM}/BattlePetInfo.cpp

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
    ${SOURCE_UTILS}/ZLibUtils.cpp
    ${SOURCE_UTILS}/ConfigDB.cpp
    ${SOURCE_UTILS}/GrowUtils.cpp
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
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Runtime"
)

target_include_directories(WorldRenderer PRIVATE
    ${SOURCE_ROOT}
    ${WORLD_RENDERER_ROOT}
    ${THIRD_PARTY_BLEND2D}
)

target_link_libraries(WorldRenderer PRIVATE
    blend2d::blend2d
)

if(WIN32)
    target_link_libraries(WorldRenderer PRIVATE
        winmm ws2_32
    )
    target_compile_definitions(WorldRenderer PRIVATE WIN32_LEAN_AND_MEAN)
endif(WIN32)