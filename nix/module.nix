{ config, pkgs, lib, utils, ... }: with lib; let
    cfg = config.programs.CaptureCardRelay;
    defaultPackage = pkgs.callPackage ./package.nix {};

    frameLimitModes = [ "camera" "vsync" "vsyncadaptive" "fps" "none" ];
    displayModes = [ "contain" "cover" "fill" "none" ];
    pixelFormats = [ "camera" "rgb24" ];

    configGenerator = (val: with lib; generators.toKeyValue {
        mkKeyValue = key: value: let
            realKey =
                if key == "fps" then "frameLimitFPS"
                else if key == "frameLimiting" then "frameLimitType"
                else key;

            valueStr =
                if key == "displayMode" then toString (lists.findFirstIndex (v: v == value) null displayModes)
                else if key == "pixelFormat" then toString (lists.findFirstIndex (v: v == value) null pixelFormats)
                else if key == "frameLimiting" then toString (lists.findFirstIndex (v: v == value) null frameLimitModes)
                else generators.mkValueStringDefault {} value;
        in "${realKey}:${valueStr}";
    } (lib.filterAttrsRecursive (n: v: v != null) val));
in {
    options.programs.CaptureCardRelay = with types; {
        enable = mkEnableOption "CaptureCardRelay, see, and hear capture card output!";
        package = mkOption {
            type = package;
            default = defaultPackage;
        };

        settings = mkOption {
            default = {};
            description = ''
                Settings for the config file, any that are not null cannot change through user settings.
            '';

            example = { fullscreen = true; };

            type = submodule (settings: {
                options.camera          = mkOption { type = nullOr str; default = null; example = "Live Gamer MINI"; };
                options.recordingDevice = mkOption { type = nullOr str; default = null; example = "Live Gamer MINI Analog Stereo"; };
                options.displayMode     = mkOption { type = nullOr (enum displayModes); default = null; example = "contain"; };
                options.pixelFormat     = mkOption { type = nullOr (enum pixelFormats); default = null; example = "rgb24"; };            
                options.frameLimiting   = mkOption { type = nullOr (enum frameLimitModes); default = null; example = "camera"; };
                options.fps             = mkOption { type = nullOr (numbers.between 0.0 250.0); default = null; description = "This option is only used when the frameLimiting mode is \"fps\""; example = 60.0; };
                options.fullscreen      = mkOption { type = nullOr bool; default = null; example = true; };
                options.volume          = mkOption { type = nullOr (ints.between 0 150); default = null; example = 100; };
            });
        };
    };

    config = mkIf cfg.enable {
        environment.systemPackages = [
            (cfg.package.override {
                commandLineArgs = [
                    (pkgs.writeTextFile {
                        name = "CaptureCardRelaySettings";
                        text = (configGenerator cfg.settings);
                    })
                ];
            })
        ];
    };
}