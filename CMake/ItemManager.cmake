cmake_minimum_required(VERSION 3.20)
project(ItemManager)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

add_definitions(-DSOCKET_USE_TLS)

include(${CMAKE_CURRENT_LIST_DIR}/Source.cmake)
set(ITEM_MANAGER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../ItemManager")

set(ITEM_MANAGER_FILES PRIVATE
    ${SOURCE_NETWORK}/NetClient.cpp
    ${SOURCE_NETWORK}/NetHTTP.cpp  
    ${SOURCE_NETWORK}/NetSocket.cpp

    ${SOURCE_ITEM}/ItemInfo.cpp
    ${SOURCE_ITEM}/ItemInfoManager.cpp
    ${SOURCE_ITEM}/ItemUtils.cpp

    ${ITEM_MANAGER_ROOT}/Main.cpp
)

add_executable(ItemManager)
add_default_sources(ItemManager)
target_sources(ItemManager PRIVATE ${ITEM_MANAGER_FILES})

find_package(OpenSSL REQUIRED)

set_target_properties(ItemManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../Runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../Runtime"
)

target_include_directories(ItemManager PRIVATE
    ${SOURCE_ROOT}
    ${ITEM_MANAGER_ROOT}
    ${THIRD_PARTY_NLOHMANN}/include
    ${THIRD_PARTY_CONCURRENTQUEUE}/include
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ItemManager PRIVATE
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
)

if(WIN32)
    target_link_libraries(ItemManager PRIVATE
        winmm ws2_32
    )
endif(WIN32)