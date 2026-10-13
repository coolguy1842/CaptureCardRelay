if(WIN32 AND NOT EMSCRIPTEN)
    set(APP_RCS ${CMAKE_CURRENT_BINARY_DIR}/${App_Branding_WindowsRC})
endif()

add_executable(${App_Name} WIN32
    clay_renderer_SDL3.cpp    
    
    frame_limiter.cpp
    settings.cpp
    
    Application/main.cpp
    Application/events.cpp

    Application/Media/camera.cpp
    Application/Media/Pipewire/audio.cpp
    Application/Media/SDL3/playback.cpp
    Application/Media/SDL3/recording.cpp

    Application/UI/main.cpp
    Application/UI/status.cpp
    Application/UI/settings.cpp

    main.cpp
    ${APP_RCS}
)

# https://stackoverflow.com/a/51190178
target_compile_definitions(${App_Name} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
target_compile_features(${App_Name} PRIVATE cxx_std_23)
target_compile_options(${App_Name} PRIVATE
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

target_include_directories(${App_Name} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/ext/include)
target_include_directories(${App_Name} PRIVATE ${PROJECT_SOURCE_DIR}/include)

if(EMSCRIPTEN)
    message(STATUS "Using Emscripten")
            
    target_compile_options(${App_Name} PRIVATE -sUSE_SDL=3 --use-port=sdl3_ttf)
    target_link_options(${App_Name} PRIVATE -sUSE_SDL=3 --use-port=sdl3_ttf)
    
    target_link_options(${App_Name} PRIVATE -sALLOW_MEMORY_GROWTH --shell-file ${PROJECT_SOURCE_DIR}/dist/shell.html -o ${App_BINARY_NAME}.html)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    if(USE_PIPEWIRE)
        target_compile_definitions(${App_Name} PUBLIC -DENABLE_PIPEWIRE)
        target_include_directories(${App_Name} PUBLIC ${PKG_PipeWire_INCLUDE_DIRS})
        target_link_libraries(${App_Name} PUBLIC ${PKG_PipeWire_LIBRARIES} rohrkabel)
    endif()

    target_link_libraries(${App_Name} PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf)
endif()

if(DEFINED App_BINARY_NAME)
    set_target_properties(${App_Name} PROPERTIES OUTPUT_NAME "${App_BINARY_NAME}")
endif()

if(DEFINED App_BINARY_RPATH)
    SET_TARGET_PROPERTIES(${App_Name} PROPERTIES INSTALL_RPATH "${App_BINARY_RPATH}")
endif()

install(TARGETS ${App_Name}
    BUNDLE DESTINATION "." COMPONENT Runtime
)

# install pdbs
if(CMAKE_CXX_LINKER_SUPPORTS_PDB)
    install(FILES $<TARGET_PDB_FILE:${Launcher_Name}> DESTINATION ${BINARY_DEST_DIR} OPTIONAL)
endif()