cmake_minimum_required(VERSION 3.25)

file(READ manifest.json MANIFEST_JSON)
string(JSON PROJ_NAME    GET "${MANIFEST_JSON}" name)
string(JSON PROJ_VERSION GET "${MANIFEST_JSON}" version)

project(${PROJ_NAME} LANGUAGES CXX VERSION ${PROJ_VERSION})

option(enable_pipewire "Enable Pipewire Direct Audio" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

if(DEFINED EMSCRIPTEN)
    set(TARGET_NAME index)
else()
    set(TARGET_NAME ${PROJECT_NAME})
endif()

include_directories(SYSTEM ext/include)
add_executable(${TARGET_NAME}
    ext/src/clay_renderer_SDL3.cpp    
    
    src/frame_limiter.cpp
    src/settings.cpp
    
    src/Application/main.cpp
    src/Application/events.cpp

    src/Application/Media/camera.cpp
    src/Application/Media/Pipewire/audio.cpp
    src/Application/Media/SDL3/playback.cpp
    src/Application/Media/SDL3/recording.cpp

    src/Application/UI/main.cpp
    src/Application/UI/status.cpp
    src/Application/UI/settings.cpp

    src/main.cpp
)

# https://stackoverflow.com/a/51190178
target_compile_definitions(${TARGET_NAME} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
target_compile_options(${TARGET_NAME} PRIVATE
	-Wall
    -Wextra
    -Werror
    -Wno-psabi
    -Wno-unknown-warning-option
    -pedantic-errors

    -Wcast-qual
    -Wdisabled-optimization
    -Wformat=2
    -Winit-self
    -Wlogical-op
    -Wmissing-include-dirs
    -Wredundant-decls
    -Wshadow
    -Wsign-conversion
    -Wswitch-default
    -Wundef
    -Wtrampolines
    -Wmaybe-uninitialized

    -fstrict-flex-arrays=3
    -Wno-missing-field-initializers

    -Wno-experimental
)

if(DEFINED EMSCRIPTEN)
    message(STATUS "Using Emscripten")
            
    target_compile_options(${TARGET_NAME} PRIVATE -sUSE_SDL=3 --use-port=sdl3_ttf)
    target_link_options(${TARGET_NAME} PRIVATE -sUSE_SDL=3 --use-port=sdl3_ttf)
    
    target_link_options(${TARGET_NAME} PRIVATE -sALLOW_MEMORY_GROWTH --shell-file ${PROJECT_SOURCE_DIR}/dist/shell.html -o index.html)

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    if(enable_pipewire)
        # https://github.com/PolisanTheEasyNick/Crescendo/blob/main/CMakeLists.txt
        find_package(PkgConfig)

        if(PkgConfig_FOUND)
            pkg_search_module(PKG_PipeWire QUIET libpipewire-0.3 pipewire-0.3)

            if(PKG_PipeWire_FOUND)
                FetchContent_Declare(
                    rohrkabel
                    GIT_REPOSITORY "https://github.com/Curve/rohrkabel"
                    GIT_TAG v13.1.1
                    SYSTEM
                )

                FetchContent_MakeAvailable(rohrkabel)

                target_compile_definitions(${TARGET_NAME} PUBLIC -DENABLE_PIPEWIRE)
                target_include_directories(${TARGET_NAME} PRIVATE ${PKG_PipeWire_INCLUDE_DIRS})
                target_link_libraries(${TARGET_NAME} PRIVATE ${PKG_PipeWire_LIBRARIES} rohrkabel)
            else()
                message("Pipewire enabled, could not find Pipewire, using SDL3 only")
            endif()
        else()
            message("Pipewire enabled, could not find PkgConfig, using SDL3 only")
        endif()
    endif()

    find_package(SDL3 QUIET)
    find_package(SDL3_ttf QUIET)

    if(NOT SDL3_FOUND)
        message("SDL3 not found, using FetchContent")

        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.18
        )

        FetchContent_MakeAvailable(SDL3)
    endif()

    if(NOT SDL3_ttf_FOUND)
        message("SDL3_ttf not found, using FetchContent")

        FetchContent_Declare(
            SDL3_ttf
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
            GIT_TAG release-3.2.2
        )

        FetchContent_MakeAvailable(SDL3_ttf)
    endif()

    if(WIN32)
        target_compile_options(${TARGET_NAME} PRIVATE mwindows)
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
endif()

target_include_directories(${TARGET_NAME} PRIVATE include)