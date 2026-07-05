project('capturecardrelay', 'cpp', 'c', version: '1.9.1', default_options: [ 'cpp_std=c++20', 'c_std=c99' ])

if get_option('debug')
    add_project_arguments('-DDEBUG', language: 'cpp')
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
    dependencies: [
        dependency('sdl3'),
        dependency('sdl3-ttf'),
        dependency('sdl3-image')
    ]
)