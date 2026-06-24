{ pkgs, ... }: pkgs.mkShell {
    hardeningDisable = [ "all" ];

    nativeBuildInputs = with pkgs; [
        clang-tools
        
        llvmPackages_latest.lldb
        llvmPackages_latest.libllvm
        llvmPackages_latest.libcxx
        llvmPackages_latest.clang

        pkg-config
        ninja
        cmake
        meson
    ];

    buildInputs = with pkgs; [
        sdl3
        sdl3-ttf
        sdl3-image
    ];
}