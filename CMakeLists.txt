cmake_minimum_required(VERSION 3.16)

project(lsw LANGUAGES C)

set(LSW_SOURCES
    src/shell/main.c
    src/common/CheckCmd.c
    src/common/parse_utils.c
    src/common/s_math.c
    src/common/str_utils.c
    src/common/sys_utils.c
    src/common/terminal.c
    src/common/utils.c
)

file(GLOB_RECURSE LSW_BUILTIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shell/builtins/*.c"
)

add_executable(lsw
    ${LSW_SOURCES}
    ${LSW_BUILTIN_SOURCES}
)

target_include_directories(lsw PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/includes"
    "${CMAKE_CURRENT_SOURCE_DIR}/includes/common"
)

if(MSVC)
    target_compile_definitions(lsw PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(lsw PRIVATE /W4 /utf-8)
else()
    target_compile_options(lsw PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(lsw PRIVATE m)
endif()
