{ system, config, pkgs, lib, ... }: {
    imports = [ ./module.nix ];

    nixpkgs.hostPlatform = "x86_64-linux";
    system.stateVersion = "26.05";

    boot = {
        enableContainers = false;

        isContainer = true;
        isNspawnContainer = true;
    };

    networking = {
        hostName = "capturecardrelay-container";
        useDHCP = false;
    };

    programs.CaptureCardRelay = {
        enable = true;
    };
}