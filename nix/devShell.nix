{ pkgs, ... }: pkgs.mkShell {
    hardeningDisable = [ "all" ];

    nativeBuildInputs = with pkgs; [
        # uncomment to use emscripten with clangd
        # (lib.hiPrio (
        #     writeShellScriptBin "clangd" ''
        #         ${llvmPackages_latest.clang-unwrapped}/bin/clangd --query-driver="${emscripten}/bin/*" "$@"
        #     ''
        # ))

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
        emscripten

        sdl3
        sdl3-ttf
    ];
}