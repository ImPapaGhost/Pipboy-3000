cmake_minimum_required(VERSION 3.21)

project(PipBoy3000 VERSION 0.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(PIPBOY_SOURCES
    Pipboy3000.c
    animations.c
    events.c
    input.c
    inventory.c
    pipboy.c
    render.c
    resources.c
    state.c
    ui.c
    MAP/map.c
)

function(pipboy_enable_warnings target)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wformat=2
        )
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()

add_library(pipboy_sdl INTERFACE)

set(PIPBOY_VENDORED_SDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDL2-devel-2.30.11-mingw/SDL2-2.30.11/x86_64-w64-mingw32")
set(PIPBOY_VENDORED_IMAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_image-devel-2.8.4-mingw/SDL2_image-2.8.4/x86_64-w64-mingw32")
set(PIPBOY_VENDORED_MIXER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_mixer-devel-2.8.0-mingw/SDL2_mixer-2.8.0/x86_64-w64-mingw32")
set(PIPBOY_VENDORED_TTF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf-devel-2.24.0-mingw/SDL2_ttf-2.24.0/x86_64-w64-mingw32")

if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "GNU" AND EXISTS "${PIPBOY_VENDORED_SDL_ROOT}/include/SDL2/SDL.h")
    target_include_directories(pipboy_sdl SYSTEM INTERFACE
        "${PIPBOY_VENDORED_SDL_ROOT}/include"
        "${PIPBOY_VENDORED_SDL_ROOT}/include/SDL2"
        "${PIPBOY_VENDORED_IMAGE_ROOT}/include"
        "${PIPBOY_VENDORED_MIXER_ROOT}/include"
        "${PIPBOY_VENDORED_TTF_ROOT}/include"
    )
    target_link_directories(pipboy_sdl INTERFACE
        "${PIPBOY_VENDORED_SDL_ROOT}/lib"
        "${PIPBOY_VENDORED_IMAGE_ROOT}/lib"
        "${PIPBOY_VENDORED_MIXER_ROOT}/lib"
        "${PIPBOY_VENDORED_TTF_ROOT}/lib"
    )
    target_link_libraries(pipboy_sdl INTERFACE
        mingw32
        SDL2main
        SDL2
        SDL2_image
        SDL2_mixer
        SDL2_ttf
    )
    set(PIPBOY_RUNTIME_DLLS
        "${CMAKE_CURRENT_SOURCE_DIR}/SDL2.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_image.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_mixer.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf.dll"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
    pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
    pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
    target_link_libraries(pipboy_sdl INTERFACE
        PkgConfig::SDL2
        PkgConfig::SDL2_IMAGE
        PkgConfig::SDL2_MIXER
        PkgConfig::SDL2_TTF
    )
endif()

add_executable(PipBoy3000 WIN32 ${PIPBOY_SOURCES})
target_include_directories(PipBoy3000 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(PipBoy3000 PRIVATE pipboy_sdl)
pipboy_enable_warnings(PipBoy3000)

set_target_properties(PipBoy3000 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(PIPBOY_RUNTIME_DLLS)
    foreach(runtime_dll IN LISTS PIPBOY_RUNTIME_DLLS)
        add_custom_command(TARGET PipBoy3000 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dll}"
                "$<TARGET_FILE_DIR:PipBoy3000>"
        )
    endforeach()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(pipboy_core_tests
        tests/core_tests.c
        events.c
        input.c
        inventory.c
        state.c
        MAP/map.c
    )
    target_compile_definitions(pipboy_core_tests PRIVATE SDL_MAIN_HANDLED)
    target_include_directories(pipboy_core_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(pipboy_core_tests PRIVATE pipboy_sdl)
    pipboy_enable_warnings(pipboy_core_tests)

    if(PIPBOY_RUNTIME_DLLS)
        foreach(runtime_dll IN LISTS PIPBOY_RUNTIME_DLLS)
            add_custom_command(TARGET pipboy_core_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${runtime_dll}"
                    "$<TARGET_FILE_DIR:pipboy_core_tests>"
            )
        endforeach()
    endif()

    add_test(NAME core_state_and_navigation COMMAND pipboy_core_tests)
    set_tests_properties(core_state_and_navigation PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()
