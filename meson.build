project('capturecardrelay', 'cpp')

qt6 = import('qt6')

relay_headers = files(
    'include/Config.hpp',
    'include/MainWindow.hpp',
)

relay_sources = files(
    'src/config.cpp',
    'src/main_window.cpp',

    'src/main.cpp',
)

relay_compiled_moc = qt6.compile_moc(
    headers: relay_headers
)

executable(
    'CaptureCardRelay',
    sources: [
        relay_compiled_moc,
        relay_sources
    ],
    include_directories: include_directories('include'),
    dependencies: [
        dependency('qt6', modules: [ 'Core', 'Gui', 'Widgets', 'Multimedia', 'MultimediaWidgets' ])
    ]
)