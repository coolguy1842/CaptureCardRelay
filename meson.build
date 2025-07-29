project('capturecardrelay', 'cpp')

executable(
    'CaptureCardRelay',
    sources: [
        'src/settings.cpp',

        'src/application/main.cpp',
        'src/application/events.cpp',
        'src/application/camera.cpp',
        'src/application/audio/playback.cpp',
        'src/application/audio/recording.cpp',
        
        'src/main.cpp'
    ],
    include_directories: include_directories('include'),
    dependencies: [
        dependency('sdl3'),
        dependency('sdl3-ttf'),
        dependency('imgui')
    ]
)