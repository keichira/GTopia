cmake_minimum_required(VERSION 3.20)
project(Build)

if(NOT DEFINED SERVER_TO_BUILD)
    set(SERVER_TO_BUILD "Master")
endif()

message(STATUS "Building server: ${SERVER_TO_BUILD}")

if(SERVER_TO_BUILD STREQUAL "Master")
    include(Master.cmake)
elseif(SERVER_TO_BUILD STREQUAL "GameServer")
    include(GameServer.cmake)
endif(SERVER_TO_BUILD STREQUAL "Master")
