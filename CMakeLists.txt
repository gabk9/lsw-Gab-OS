cmake_minimum_required(VERSION 3.10)
project(meu_projeto C)

set(SOURCES
    main.c
    libs/utils.c
    libs/s_math.c
    libs/CheckCmd.c
    libs/terminal.c
)

include_directories(libs)

add_executable(meu_programa ${SOURCES})

if(UNIX AND NOT APPLE)
    target_link_libraries(meu_programa m)
endif()

if(WIN32)
    set_target_properties(meu_programa PROPERTIES OUTPUT_NAME "main.exe")
else()
    set_target_properties(meu_programa PROPERTIES OUTPUT_NAME "main")
endif()