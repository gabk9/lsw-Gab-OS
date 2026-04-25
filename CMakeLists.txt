cmake_minimum_required(VERSION 3.10)
project(shell C)

set(SOURCES
    main.c 
    libs/utils.c
    libs/s_math.c
    libs/CheckCmd.c
    libs/terminal.c
)

include_directories(libs)

add_executable(lsw ${SOURCES})

if(WIN32)
    if(MSVC)
        target_compile_definitions(lsw PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
else()
    target_link_libraries(lsw m)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lsw PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(lsw PRIVATE /W4)
endif()