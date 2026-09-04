{ pkgs, lib, commandLineArgs ? "", ... }: pkgs.stdenv.mkDerivation {
    name = "CaptureCardRelay";
    version = "1.10.0";
    src = ../.;

    nativeBuildInputs = with pkgs; [
        clang
        pkg-config
        ninja
        cmake
        meson

        makeWrapper
    ];

    buildInputs = with pkgs; [
        sdl3
        sdl3-ttf
        sdl3-image
        imgui
    ];

    configurePhase = ''
        meson setup build -Dprefix=$out -Dbuildtype=release
    '';

    buildPhase = ''
        meson compile -C build
    '';

    installPhase = ''
        mkdir -p $out/bin
        mkdir -p $out/share/applications
        mkdir -p $out/share/icons/hicolor/64x64/apps
        
        cp build/CaptureCardRelay $out/bin/
        wrapProgram $out/bin/CaptureCardRelay \
            --add-flags ${lib.escapeShellArg commandLineArgs}

        cp assets/capture-card-relay.desktop $out/share/applications
        cp assets/capture-card-relay.png $out/share/icons/hicolor/64x64/apps
    '';

    meta = with lib; {
        description = "Viewer for capture cards";
        homepage = "https://github.com/coolguy1842/CaptureCardRelay/";
        license = licenses.gpl3;
    };
}