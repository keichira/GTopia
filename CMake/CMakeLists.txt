cmake_minimum_required(VERSION 3.20)
project(Build)

if(NOT DEFINED PROJECT_TO_BUILD)
    set(PROJECT_TO_BUILD "Master")
endif()

message(STATUS "Building project: ${SERVER_TO_BUILD}")

if(PROJECT_TO_BUILD STREQUAL "Master")
    include(Master.cmake)
elseif(PROJECT_TO_BUILD STREQUAL "GameServer")
    include(GameServer.cmake)
elseif(PROJECT_TO_BUILD STREQUAL "ItemManager")
    include(ItemManager.cmake)
endif()
