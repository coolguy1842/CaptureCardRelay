{
    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable"; 
        flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system: let
        pkgs = nixpkgs.legacyPackages.${system};
    in {
        devShells = {
            default = import ./nix/devShell.nix { inherit pkgs; };
        };

        packages = rec {
            CaptureCardRelay = pkgs.callPackage ./nix/package.nix {};
            default = CaptureCardRelay;
        };

        nixosModules = rec {
            CaptureCardRelay = (import ./nix/module.nix);
            default = CaptureCardRelay;
        };
    }) //
    flake-utils.lib.eachDefaultSystemPassThrough (system: {
        nixosConfigurations.container = nixpkgs.lib.nixosSystem {
            modules = [ ./nix/container.nix ];
        };
    });
}
