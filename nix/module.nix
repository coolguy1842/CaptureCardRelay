{ config, pkgs, lib, utils, ... }: with lib; let
    cfg = config.programs.CaptureCardRelay;
    defaultPackage = pkgs.callPackage ./package.nix {};

    frameLimitModes = [ "camera" "vsync" "vsyncadaptive" "fps" "none" ];
    displayModes = [ "contain" "cover" "fill" "none" ];
    pixelFormats = [ "camera" "rgb24" ];
    scaleModes = [ "nearest" "linear" "pixelart" ];

    sdlPixelFormats = [ "SDL_PIXELFORMAT_UNKNOWN" "SDL_PIXELFORMAT_INDEX1LSB" "SDL_PIXELFORMAT_INDEX1MSB" "SDL_PIXELFORMAT_INDEX2LSB" "SDL_PIXELFORMAT_INDEX2MSB" "SDL_PIXELFORMAT_INDEX4LSB" "SDL_PIXELFORMAT_INDEX4MSB" "SDL_PIXELFORMAT_INDEX8" "SDL_PIXELFORMAT_RGB332" "SDL_PIXELFORMAT_XRGB4444" "SDL_PIXELFORMAT_XBGR4444" "SDL_PIXELFORMAT_XRGB1555" "SDL_PIXELFORMAT_XBGR1555" "SDL_PIXELFORMAT_ARGB4444" "SDL_PIXELFORMAT_RGBA4444" "SDL_PIXELFORMAT_ABGR4444" "SDL_PIXELFORMAT_BGRA4444" "SDL_PIXELFORMAT_ARGB1555" "SDL_PIXELFORMAT_RGBA5551" "SDL_PIXELFORMAT_ABGR1555" "SDL_PIXELFORMAT_BGRA5551" "SDL_PIXELFORMAT_RGB565" "SDL_PIXELFORMAT_BGR565" "SDL_PIXELFORMAT_RGB24" "SDL_PIXELFORMAT_BGR24" "SDL_PIXELFORMAT_XRGB8888" "SDL_PIXELFORMAT_RGBX8888" "SDL_PIXELFORMAT_XBGR8888" "SDL_PIXELFORMAT_BGRX8888" "SDL_PIXELFORMAT_ARGB8888" "SDL_PIXELFORMAT_RGBA8888" "SDL_PIXELFORMAT_ABGR8888" "SDL_PIXELFORMAT_BGRA8888" "SDL_PIXELFORMAT_XRGB2101010" "SDL_PIXELFORMAT_XBGR2101010" "SDL_PIXELFORMAT_ARGB2101010" "SDL_PIXELFORMAT_ABGR2101010" "SDL_PIXELFORMAT_RGB48" "SDL_PIXELFORMAT_BGR48" "SDL_PIXELFORMAT_RGBA64" "SDL_PIXELFORMAT_ARGB64" "SDL_PIXELFORMAT_BGRA64" "SDL_PIXELFORMAT_ABGR64" "SDL_PIXELFORMAT_RGB48_FLOAT" "SDL_PIXELFORMAT_BGR48_FLOAT" "SDL_PIXELFORMAT_RGBA64_FLOAT" "SDL_PIXELFORMAT_ARGB64_FLOAT" "SDL_PIXELFORMAT_BGRA64_FLOAT" "SDL_PIXELFORMAT_ABGR64_FLOAT" "SDL_PIXELFORMAT_RGB96_FLOAT" "SDL_PIXELFORMAT_BGR96_FLOAT" "SDL_PIXELFORMAT_RGBA128_FLOAT" "SDL_PIXELFORMAT_ARGB128_FLOAT" "SDL_PIXELFORMAT_BGRA128_FLOAT" "SDL_PIXELFORMAT_ABGR128_FLOAT" "SDL_PIXELFORMAT_YV12" "SDL_PIXELFORMAT_IYUV" "SDL_PIXELFORMAT_YUY2" "SDL_PIXELFORMAT_UYVY" "SDL_PIXELFORMAT_YVYU" "SDL_PIXELFORMAT_NV12" "SDL_PIXELFORMAT_NV21" "SDL_PIXELFORMAT_P010" "SDL_PIXELFORMAT_EXTERNAL_OES" "SDL_PIXELFORMAT_MJPG" ];
    sdlColorspaces = [ "SDL_COLORSPACE_SRGB" "SDL_COLORSPACE_SRGB_LINEAR" "SDL_COLORSPACE_HDR10" "SDL_COLORSPACE_JPEG" "SDL_COLORSPACE_BT601_LIMITED" "SDL_COLORSPACE_BT601_FULL" "SDL_COLORSPACE_BT709_LIMITED" "SDL_COLORSPACE_BT709_FULL" "SDL_COLORSPACE_BT2020_LIMITED" "SDL_COLORSPACE_BT2020_FULL" ];

    configGenerator = (val: with lib; generators.toKeyValue {
        mkKeyValue = key: value: let
            realKey =
                if key == "fps" then "frameLimitFPS"
                else if key == "frameLimiting" then "frameLimitType"
                else key;

            valueStr =
                if key == "displayMode" then toString (lists.findFirstIndex (v: v == value) null displayModes)
                else if key == "scaleMode" then toString (lists.findFirstIndex (v: v == value) null scaleModes)
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
                options.camera               = mkOption { type = nullOr str;                         default = null; example = "Live Gamer MINI"; };
                options.recordingDevice      = mkOption { type = nullOr str;                         default = null; example = "Live Gamer MINI Analog Stereo"; };
                options.displayMode          = mkOption { type = nullOr (enum displayModes);         default = null; example = "contain"; };
                options.scaleMode            = mkOption { type = nullOr (enum scaleModes);           default = null; example = "linear"; };
                options.pixelFormat          = mkOption { type = nullOr (enum pixelFormats);         default = null; example = "rgb24"; };            
                options.frameLimiting        = mkOption { type = nullOr (enum frameLimitModes);      default = null; example = "camera"; };
                options.fps                  = mkOption { type = nullOr (numbers.between 0.0 250.0); default = null; example = 60.0; description = "This option is only used when the frameLimiting mode is \"fps\""; };
                options.fullscreen           = mkOption { type = nullOr bool;                        default = null; example = true; };
                options.volume               = mkOption { type = nullOr (ints.between 0 150);        default = null; example = 100; };
                options.preferredPixelFormat = mkOption { type = nullOr (enum sdlPixelFormats);      default = null; example = "SDL_PIXELFORMAT_NV12"; };
                options.preferredColorspace  = mkOption { type = nullOr (enum sdlColorspaces);       default = null; example = "SDL_COLORSPACE_SRGB"; };
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