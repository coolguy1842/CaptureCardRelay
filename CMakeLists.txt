cmake_minimum_required(VERSION 3.27)

file(READ manifest.json MANIFEST_JSON)
string(JSON PROJ_NAME    GET "${MANIFEST_JSON}" name)
string(JSON PROJ_VERSION GET "${MANIFEST_JSON}" version)

project(${PROJ_NAME} LANGUAGES CXX VERSION ${PROJ_VERSION})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# https://github.com/PrismLauncher/PrismLauncher/blob/6f5723937f558b06eb6795d16e0994299ba9c040/CMakeLists.txt#L43
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    add_compile_options(
        # /GS Adds buffer security checks, default on but included anyway to mirror gcc's fstack-protector flag
        "$<$<COMPILE_LANGUAGE:C,CXX>:/GS>"

        # /Gw helps reduce binary size
        # /Gy allows the compiler to package individual functions
        # /guard:cf enables control flow guard
        "$<$<AND:$<CONFIG:Release,RelWithDebInfo>,$<COMPILE_LANGUAGE:C,CXX>>:/Gw;/Gy;/guard:cf>"
    )

    add_link_options(
        # /LTCG allows for linking wholy optimizated programs
        "$<$<COMPILE_LANGUAGE:C,CXX>:/LTCG>"
    )

    # /GL enables whole program optimizations
    # NOTE: With Clang, this is implemented as regular LTO and only used during linking
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options("$<$<AND:$<CONFIG:Release,RelWithDebInfo>,$<COMPILE_LANGUAGE:C,CXX>>:/GL>")
    endif()

    # See https://github.com/ccache/ccache/issues/1040
    # TODO(@getchoo): Is sccache affected by this? Would be nice to use `ProgramDatabase`....
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

    # Ensure our runtime library matches our vcpkg dependencies
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

    # TODO(@getchoo): Do we actually need this???
    if(CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDLL")
        set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release "")
        set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release "")
    endif()
else()
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-fstack-protector-strong;--param=ssp-buffer-size=4>")

    # Avoid re-defining _FORTIFY_SOURCE, as it can cause redefinition errors in setups that use it by default (i.e., package builds)
    if(NOT (CMAKE_C_FLAGS MATCHES "-D_FORTIFY_SOURCE" OR CMAKE_CXX_FLAGS MATCHES "-D_FORTIFY_SOURCE"))
        # NOTE: _FORTIFY_SOURCE requires optimizations in most newer versions of glibc
        add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release,RelWithDebInfo>>:-D_FORTIFY_SOURCE=2>")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # -ffunction-sections and -fdata-sections help reduce binary size
        # -mguard=cf enables Control Flow Guard
        # TODO: Look into -gc-sections to further reduce binary size
        add_compile_options("$<$<AND:$<CONFIG:Release,RelWithDebInfo>,$<COMPILE_LANGUAGE:C,CXX>>:-ffunction-sections;-fdata-sections;-mguard=cf>")
    endif()
endif()

if(CMAKE_GENERATOR STREQUAL "Unix Makefiles" OR CMAKE_GENERATOR MATCHES "^Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(USE_PIPEWIRE "Enable the use of Pipewire direct audio" ON)
option(USE_CLANG_TIDY "Enable the use of clang-tidy during compilation" OFF)
option(DEBUG_ADDRESS_SANITIZER "Enable Address Sanitizer in Debug builds" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)

string(TIMESTAMP TODAY "%Y-%m-%d")
set(App_BUILD_TIMESTAMP "${TODAY}")

if(EMSCRIPTEN)
    set(App_BINARY_NAME "index" CACHE STRING "Name of the output HTML, JS, WASM file")
else()
    set(App_BINARY_NAME "CaptureCardRelay" CACHE STRING "Name of the application binary")
endif()

add_subdirectory(program_info)

# https://github.com/PrismLauncher/PrismLauncher/blob/60bcc2694d7433e2d2a657fbf62b0d022f9116e0/CMakeLists.txt#L121
if(DEBUG_ADDRESS_SANITIZER)
    message(STATUS "Address Sanitizer enabled for Debug builds, Turn it off with -DDEBUG_ADDRESS_SANITIZER=off")

    set(USE_ASAN_COMPILE_OPTIONS $<AND:$<CONFIG:Debug,RelWithDebInfo>,$<COMPILE_LANGUAGE:C,CXX>>)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        message(STATUS "Using Address Sanitizer compile options for MSVC frontend")
        add_compile_options(
            $<${USE_ASAN_COMPILE_OPTIONS}:/fsanitize=address>
            $<${USE_ASAN_COMPILE_OPTIONS}:/Oy->
        )
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        message(STATUS "Using Address Sanitizer compile options for GCC/Clang")
        add_compile_options(
            $<${USE_ASAN_COMPILE_OPTIONS}:-fsanitize=address,undefined>
            $<${USE_ASAN_COMPILE_OPTIONS}:-fno-omit-frame-pointer>
            $<${USE_ASAN_COMPILE_OPTIONS}:-fno-sanitize-recover=null>
        )
        add_link_options($<${USE_ASAN_COMPILE_OPTIONS}:-fsanitize=address,undefined>)
    else()
        message(STATUS "Address Sanitizer not available on compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# https://github.com/PrismLauncher/PrismLauncher/blob/60bcc2694d7433e2d2a657fbf62b0d022f9116e0/CMakeLists.txt#L147
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)

        if(CMAKE_BUILD_TYPE)
            if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
                message(STATUS "IPO / LTO enabled")
            else()
                message(STATUS "Not enabling IPO / LTO on debug builds")
            endif()
        else()
            message(STATUS "IPO / LTO will only be enabled for release builds")
        endif()
    else()
        message(STATUS "IPO / LTO not supported: <${ipo_error}>")
    endif()
endif()

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
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${App_MetaInfo} DESTINATION ${KDE_INSTALL_METAINFODIR})
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
endif()

# clang doesnt like the --use-port and -sUSE_SDL=3 options
# https://github.com/PrismLauncher/PrismLauncher/blob/60bcc2694d7433e2d2a657fbf62b0d022f9116e0/CMakeLists.txt#L107
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