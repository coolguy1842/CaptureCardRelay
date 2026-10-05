{
    pkgs, lib,
    commandLineArgs ? "",
    pipewireSupport ? true,
    ...
}: let
    cpm-src = pkgs.fetchurl {
        url = "https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.43.2/CPM.cmake";
        sha256 = "sha256-SaO++RzrZbtm1XJV4S0f/SKrwvZAj6n+RTTFRKLyMqo=";
    };

    rohrkabel = rec {
        version = "v13.1.1";
        src = pkgs.fetchFromGitHub {
            owner = "Curve";
            repo = "rohrkabel";
            rev = version;
            hash = "sha256-Ms85iDhA6WbIXIoPl8tQEchIDiT0vw2503wgI/LS4bY=";
        };
    };

    coco = rec {
        version = "v4.7.2";
        src = pkgs.fetchFromGitHub {
            owner = "Curve";
            repo = "coco";
            rev = version;
            hash = "sha256-Kn107UmvuTH+xKkaeL3PvpbLpPmhuIq3lhnkduSr9vg=";
        };
    };

    ereignis = rec {
        version = "v6.3.0";
        src = pkgs.fetchFromGitHub {
            owner = "Curve";
            repo = "ereignis";
            rev = version;
            hash = "sha256-UT3C63Jtg0XgF/VD69uUwtWMGJCtDTtYD/QfCxSAhwA=";
        };
    };

    channel = rec {
        version = "v4.0.0";
        src = pkgs.fetchFromGitHub {
            owner = "Curve";
            repo = "channel";
            rev = version;
            hash = "sha256-gRUxd13HACXCRZ734NUDqmQ4PaTp+6V0jSHBlKcsAzM=";
        };
    };

    packageProject-cmake = rec {
        version = "v1.13.0";
        src = pkgs.fetchFromGitHub {
            owner = "TheLartians";
            repo = "PackageProject.cmake";
            rev = version;
            hash = "sha256-RbqywBm+C1Bo1KeFL2MdsE2PMKWbc3x2iWswKGXtO9o=";
        };
    };

    manifest = (pkgs.lib.importJSON ../manifest.json);
in pkgs.stdenv.mkDerivation {
    name = manifest.name;
    version = manifest.version;
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
    ] ++ lib.optionals pipewireSupport [
        pipewire
    ];

    cmakeFlags =[
        "-Duse_pipewire=${if pipewireSupport then "ON" else "OFF"}"
        
        "-DFETCHCONTENT_SOURCE_DIR_ROHRKABEL=/tmp/rohrkabel"
        "-DCPM_coco_SOURCE=/tmp/coco"
        "-DCPM_ereignis_SOURCE=/tmp/ereignis"
        "-DCPM_channel_SOURCE=/tmp/channel"
        "-DCPM_PackageProject_SOURCE=${packageProject-cmake.src}"
    ];

    preConfigure = ''
        addDep() {
            mkdir /tmp/$1
            cp -ar $2/* /tmp/$1
            chmod -R 777 /tmp/$1
            echo "include(\"${cpm-src}\")" > /tmp/$1/cmake/cpm.cmake
        }

        addDep rohrkabel "${rohrkabel.src}"
        addDep coco "${coco.src}"
        addDep ereignis "${ereignis.src}"
        addDep channel "${channel.src}"
    '';

    installPhase = ''
        mkdir -p $out/bin
        mkdir -p $out/share/applications
        mkdir -p $out/share/icons/hicolor/64x64/apps

        cp ./${manifest.name} $out/bin/${manifest.name}
        wrapProgram $out/bin/${manifest.name} \
            --add-flags ${lib.escapeShellArg commandLineArgs}

        cp $src/assets/capture-card-relay.desktop $out/share/applications
        cp $src/assets/capture-card-relay.png $out/share/icons/hicolor/64x64/apps
    '';

    meta = with lib; {
        description = "Viewer for capture cards";
        homepage = "https://github.com/coolguy1842/CaptureCardRelay/";
        license = licenses.gpl3;

        mainProgram = manifest.name;
    };
}