{
    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable"; 
    };

    outputs = { self, nixpkgs, systems }: let 
        forEachSystem = nixpkgs.lib.genAttrs (import systems);
    in {
        devShells = forEachSystem (system: let
            pkgs = nixpkgs.legacyPackages.${system};
        in {
            default = pkgs.mkShell {
                hardeningDisable = [ "all" ];

                nativeBuildInputs = with pkgs; [
                    clang
                    pkg-config
                    ninja
                    cmake
                    meson
                ];

                buildInputs = with pkgs; [
                    pipewire
                    ffmpeg
                    qt6.full
                ];
            };
        });

        packages = forEachSystem (system: let
            pkgs = nixpkgs.legacyPackages.${system};
        in {
            default = pkgs.stdenv.mkDerivation {
                name = "CaptureCardRelay";
                version = "1.0";
                src = ./.;

                nativeBuildInputs = with pkgs; [
                    clang
                    pkg-config
                    ninja
                    cmake
                    meson
                    qt6.wrapQtAppsHook
                ];

                buildInputs = with pkgs; [
                    qt6.full
                    qt6.qtbase
                    makeWrapper
                ];

                configurePhase = ''
                    meson setup -Dprefix=$out build
                '';

                buildPhase = ''
                    meson compile -C build
                '';

                installPhase = ''
                    meson install -C build
                '';

                meta = with pkgs.lib; {
                    description = "Viewer for capture cards";
                    homepage = "https://github.com/coolguy1842/CaptureCardRelay/";
                    license = licenses.gpl3;
                };
            };
        });
    };
}
