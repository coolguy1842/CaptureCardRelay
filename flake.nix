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
                    sdl3
                    sdl3-ttf
                    sdl3-image
                    imgui
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

                # TODO: remove this later, for debugging purposes
                hardeningDisable = [ "all" ];
                nativeBuildInputs = with pkgs; [
                    clang
                    pkg-config
                    ninja
                    cmake
                    meson
                ];

                buildInputs = with pkgs; [
                    sdl3
                    sdl3-ttf
                    sdl3-image
                    imgui
                ];

                configurePhase = ''
                    meson setup -Dprefix=$out build
                '';

                buildPhase = ''
                    meson compile -C build
                '';

                installPhase = ''
                    mkdir -p $out/bin
                    cp build/CaptureCardRelay $out/bin/
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
