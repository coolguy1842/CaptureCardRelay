project('capturecardrelay', 'cpp', 'c', default_options: [ 'cpp_std=c++20', 'c_std=c99' ])

executable(
    'CaptureCardRelay',
    sources: [
        'ext/src/clay_renderer_SDL3.cpp',
        'src/settings.cpp',

        'src/application/main.cpp',
        'src/application/events.cpp',
        'src/application/render.cpp',
        'src/application/camera.cpp',
        'src/application/audio/playback.cpp',
        'src/application/audio/recording.cpp',

        'src/main.cpp'
    ],
    include_directories: [
        include_directories('include'),
        include_directories('ext/include')
    ],
    dependencies: [
        dependency('sdl3'),
        dependency('sdl3-ttf'),
        dependency('sdl3-image'),
        dependency('imgui')
    ]
)