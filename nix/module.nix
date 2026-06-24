{ config, pkgs, lib, utils, ... }: with lib; let
    cfg = config.programs.CaptureCardRelay;
    defaultPackage = pkgs.callPackage ./package.nix {};
in {
    options.programs.CaptureCardRelay = with types; {
        enable = mkEnableOption "CaptureCardRelay, see, and hear capture card output";
        package = mkOption {
            type = package;
            default = defaultPackage;
        };
    };

    config = mkIf cfg.enable {
        environment.systemPackages = [ cfg.package ];
    };
}