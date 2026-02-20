set(SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../Source")
set(SOURCE_DATABASE "${SOURCE_ROOT}/Database")
set(SOURCE_IO "${SOURCE_ROOT}/IO")
set(SOURCE_ITEM "${SOURCE_ROOT}/Item")
set(SOURCE_MATH "${SOURCE_ROOT}/Math")
set(SOURCE_MEMORY "${SOURCE_ROOT}/Memory")
set(SOURCE_NETWORK "${SOURCE_ROOT}/Network")
set(SOURCE_PACKET "${SOURCE_ROOT}/Packet")
set(SOURCE_PLAYER "${SOURCE_ROOT}/Player")
set(SOURCE_PROTON "${SOURCE_ROOT}/Proton")
set(SOURCE_SERVER "${SOURCE_ROOT}/Server")
set(SOURCE_UTILS "${SOURCE_ROOT}/Utils")
set(SOURCE_WORLD "${SOURCE_ROOT}/World")

set(THIRD_PARTY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty")
set(THIRD_PARTY_ENET "${THIRD_PARTY_ROOT}/enet")
set(THIRD_PARTY_CONCURRENTQUEUE "${THIRD_PARTY_ROOT}/concurrentqueue")

set(SOURCE_DEFAULT_FILES
    ${SOURCE_DATABASE}/DatabaseManager.cpp
    ${SOURCE_DATABASE}/DatabasePool.cpp 
    ${SOURCE_DATABASE}/DatabaseResult.cpp
    ${SOURCE_DATABASE}/DatabaseWorker.cpp
    ${SOURCE_DATABASE}/PreparedParam.cpp

    ${SOURCE_IO}/File.cpp
    ${SOURCE_IO}/Log.cpp

    ${SOURCE_MATH}/Random.cpp
    ${SOURCE_MATH}/Vector2.cpp

    ${SOURCE_MEMORY}/MemoryBuffer.cpp
    ${SOURCE_MEMORY}/RingBuffer.cpp

    ${SOURCE_NETWORK}/ENetServer.cpp 
    ${SOURCE_NETWORK}/NetClient.cpp
    ${SOURCE_NETWORK}/NetSocket.cpp
    ${SOURCE_NETWORK}/NetEntity.cpp

    ${SOURCE_PROTON}/ProtonUtils.cpp

    ${SOURCE_UTILS}/StringUtils.cpp
    ${SOURCE_UTILS}/Timer.cpp
    ${SOURCE_UTILS}/Variant.cpp

    ${SOURCE_ROOT}/ContextBase.cpp

    ${THIRD_PARTY_ENET}/callbacks.c
    ${THIRD_PARTY_ENET}/compress.c
    ${THIRD_PARTY_ENET}/host.c
    ${THIRD_PARTY_ENET}/list.c
    ${THIRD_PARTY_ENET}/packet.c
    ${THIRD_PARTY_ENET}/protocol.c
    ${THIRD_PARTY_ENET}/peer.c
    ${THIRD_PARTY_ENET}/unix.c
    ${THIRD_PARTY_ENET}/win32.c
)

if(WIN32)
    list(APPEND SOURCE_DEFAULT_FILES ${SOURCE_ROOT}/OS/Windows/WindowsPrecompiled.cpp)
else()
    list(APPEND SOURCE_DEFAULT_FILES ${SOURCE_ROOT}/OS/Linux/LinuxPrecompiled.cpp)
endif(WIN32)

function(add_default_sources target_name)
    target_sources(${target_name} PRIVATE ${SOURCE_DEFAULT_FILES})
endfunction(add_default_sources target_name)