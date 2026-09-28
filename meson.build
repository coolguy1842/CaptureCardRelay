project('capturecardrelay', 'cpp', 'c', version: '1.11.5', default_options: [ 'cpp_std=c++20', 'c_std=c99' ])

if get_option('debug')
    add_project_arguments('-DDEBUG', language: 'cpp')
endif

add_project_arguments('-DPROJECT_VERSION="' + meson.project_version() + '"', language: 'cpp')

dependencies = []
link_args = []

if host_machine.system() != 'emscripten'
    # systems distribute with underscore or dash, cater to both
    sdl3_ttf_lib = dependency('sdl3-ttf', required: false)
    if not sdl3_ttf_lib.found()
        sdl3_ttf_lib = dependency('sdl3_ttf', required: true)
    endif

    dependencies = [
        dependency('sdl3'),
        sdl3_ttf_lib
    ]
else
    link_args = [
        '--shell-file', join_paths(meson.current_source_dir(), 'dist/shell.html'),
        '-o', 'index.html',

        '-s', 'ALLOW_MEMORY_GROWTH',
    ]

    dependencies = [
        declare_dependency(
            compile_args: [ '-sUSE_SDL=3', '--use-port=sdl3_ttf' ],
            link_args: [ '-sUSE_SDL=3', '--use-port=sdl3_ttf' ]
        )
    ]
endif


executable(
    'CaptureCardRelay',
    sources: [
        'ext/src/clay_renderer_SDL3.cpp',
        
        'src/settings.cpp',
        'src/frame_limiter.cpp',

        'src/Application/main.cpp',
        'src/Application/events.cpp',

        'src/Application/Media/camera.cpp',
        'src/Application/Media/playback.cpp',
        'src/Application/Media/recording.cpp',

        'src/Application/UI/main.cpp',
        'src/Application/UI/status.cpp',
        'src/Application/UI/settings.cpp',

        'src/main.cpp'
    ],
    include_directories: [
        include_directories('include'),
        # ignore warnings from clay
        include_directories('ext/include', is_system: true)
    ],
    dependencies: dependencies,
    link_args: link_args,
    cpp_args: [
        '-Wall',
        '-Wextra',
        '-Werror',
        '-Wno-psabi',
        '-Wno-unknown-warning-option',
        '-pedantic-errors',
        
        '-Wcast-qual',
        '-Wdisabled-optimization',
        '-Wformat=2',
        '-Winit-self',
        '-Wlogical-op',
        '-Wmissing-include-dirs',
        '-Wredundant-decls',
        '-Wshadow',
        '-Wsign-conversion',
        '-Wswitch-default',
        '-Wundef',
        '-Wtrampolines',
        '-Wmaybe-uninitialized',

        '-fstrict-flex-arrays=3',
        '-Wno-missing-field-initializers',

        '-Wno-experimental'
    ],
    win_subsystem: 'windows'
)