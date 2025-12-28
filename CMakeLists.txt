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

add_executable(main ${SOURCES})

if(WIN32)
    set_target_properties(main PROPERTIES SUFFIX ".exe")
    
    if(MINGW)
    endif()
    
    if(MSVC)
        target_compile_definitions(main PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
else()
    target_link_libraries(main m)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic)
endif()