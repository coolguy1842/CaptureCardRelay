cmake_minimum_required(VERSION 3.25)

file(READ manifest.json MANIFEST_JSON)
string(JSON PROJ_NAME    GET "${MANIFEST_JSON}" name)
string(JSON PROJ_VERSION GET "${MANIFEST_JSON}" version)

project(${PROJ_NAME} LANGUAGES CXX VERSION ${PROJ_VERSION})

set(App_VERSION_NAME "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(App_VERSION_NAME4 "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0")
set(App_VERSION_NAME4_COMMA "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
add_compile_definitions("CAPTURECARDRELAY_VERSION=\"${PROJ_VERSION}\"")

include(FetchContent)

# https://github.com/PrismLauncher/PrismLauncher/blob/6cf70e75064171c0c7d756d5a111e76a3360d513/CMakeLists.txt#L24
# Output all executables and shared libs in the main build folder, not in subfolders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(UNIX)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

if(CMAKE_GENERATOR STREQUAL "Unix Makefiles" OR CMAKE_GENERATOR MATCHES "^Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(USE_PIPEWIRE "Enable the use of Pipewire direct audio" ON)
option(USE_CLANG_TIDY "Enable the use of clang-tidy during compilation" OFF)

if(EMSCRIPTEN)
    set(App_BINARY_NAME "index" CACHE STRING "Name of the output HTML, JS, WASM file")
else()
    set(App_BINARY_NAME "CaptureCardRelay" CACHE STRING "Name of the application binary")
endif()

add_subdirectory(program_info)

if(NOT EMSCRIPTEN)
    if(UNIX AND NOT APPLE)
        if(USE_PIPEWIRE)
            # https://github.com/PolisanTheEasyNick/Crescendo/blob/main/CMakeLists.txt
            find_package(PkgConfig)

            if(PkgConfig_FOUND)
                pkg_search_module(PKG_PipeWire QUIET libpipewire-0.3 pipewire-0.3)

                if(PKG_PipeWire_FOUND)
                    FetchContent_Declare(
                        rohrkabel
                        GIT_REPOSITORY "https://github.com/Curve/rohrkabel"
                        GIT_TAG v13.1.1
                        EXCLUDE_FROM_ALL
                        SYSTEM
                    )

                    FetchContent_MakeAvailable(rohrkabel)
                else()
                    set(USE_PIPEWIRE OFF)
                    message("Pipewire enabled, could not find Pipewire, using SDL3 only")
                endif()
            else()
                set(USE_PIPEWIRE OFF)
                message("Pipewire enabled, could not find PkgConfig, using SDL3 only")
            endif()
        endif()

        find_package(ECM NO_MODULE REQUIRED)
        set(CMAKE_MODULE_PATH "${ECM_MODULE_PATH};${CMAKE_MODULE_PATH}")

        include(KDEInstallDirs)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${App_Desktop} DESTINATION ${KDE_INSTALL_APPDIR})
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${App_SVG} DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps")
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${App_PNG_256} DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/256x256/apps" RENAME "${App_ID}.png")
    elseif(NOT WIN32)
        message(FATAL_ERROR "Platform not supported")
    endif()

    find_package(SDL3 QUIET)
    find_package(SDL3_ttf QUIET)

    if(NOT SDL3_FOUND)
        message("SDL3 not found, using FetchContent")

        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.18
            EXCLUDE_FROM_ALL
        )

        FetchContent_MakeAvailable(SDL3)
    endif()

    if(NOT SDL3_ttf_FOUND)
        message("SDL3_ttf not found, using FetchContent")

        FetchContent_Declare(
            SDL3_ttf
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
            GIT_TAG release-3.2.2
            EXCLUDE_FROM_ALL
        )

        FetchContent_MakeAvailable(SDL3_ttf)
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined -g")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,undefined -g")
endif()

# clang doesnt like the --use-port and -sUSE_SDL=3 options
if(USE_CLANG_TIDY AND NOT EMSCRIPTEN)
    find_program(CLANG_TIDY clang-tidy OPTIONAL)

    if(CLANG_TIDY)
        message(STATUS "Using clang-tidy during compilation")
    
        set(CLANG_TIDY_COMMAND "${CLANG_TIDY}" "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_COMMAND})
    else()
        message(WARNING "Unable to find `clang-tidy`. Not using during compilation")
    endif()
endif()

add_subdirectory(src)