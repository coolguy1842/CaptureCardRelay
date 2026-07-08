# Capture Card Relay
#### Tested on NixOS, ArchLinux, Windows

While this is meant for viewing and listening to a capture cards output, it is essentially, a camera viewer and a microphone relay.

<br>

<strong>This program is feature complete.</strong><br>
I will try fix any bugs reported, or that I find. If a feature request is given, I may implement it. 

### Controls
| Key | Action                       |
| --- | ---------------------------- |
| Left Arrow    | Cycle Cameras      |
| Right Arrow   | Cycle Microphones  |
| Up/Down Arrow | Raise/Lower Volume |
| O             | Open Settings Menu |

### Settings
Currently, there are settings for:
  - Camera
  - Microphone
  - Display Mode (Letterbox/Cover etc)
  - Pixel Format (For compatibility, Camera is fastest, but less compatible)
  - Frame Limiting (Camera is likely to be best, and is default)
  - Fullscreen
  - Volume

#### There are also hidden settings (not in GUI) for:
| Setting | Description | Example |
| ------- | ----------- | ------- |
| preferredColorspace  | Valid options are found [here](https://wiki.libsdl.org/SDL3/SDL_Colorspace) | SDL_COLORSPACE_SRGB |
| preferredPixelFormat | Which pixel format the camera outputs to the program, the other pixelFormat setting applies to the texture output. Valid options are found [here](https://wiki.libsdl.org/SDL3/SDL_PixelFormat) | SDL_PIXELFORMAT_NV12 |

### Installing<br><sub>NOTE: Others may be added if requested, otherwise build manually.</sub>

Arch:
```sh
wget https://raw.githubusercontent.com/coolguy1842/CaptureCardRelay/refs/heads/master/PKGBUILD
makepkg -si
```

Nix:
- Add the following to flake inputs 
  ```nix
  capturecardrelay = {
    url = "github:coolguy1842/CaptureCardRelay";
    inputs.nixpkgs.follows = "nixpkgs";
  };
  ```
- A: Install package with `environment.systemPackages/users.users.<user>.packages = [ inputs.capturecardrelay.packages.${pkgs.stdenv.hostPlatform.system}.default ];`
- B: Add `inputs.capturecardrelay.nixosModules.<hostPlatform>.default` to nixosSystem modules, and configure with `programs.CaptureCardRelay`. See [module.nix](https://github.com/coolguy1842/CaptureCardRelay/blob/master/nix/module.nix) for settings

### Building
```sh
git clone https://github.com/coolguy1842/CaptureCardRelay
cd CaptureCardRelay

meson setup build
meson compile -C build

./build/CaptureCardRelay
```

## Thanks To
- [SDL3](https://github.com/libsdl-org/SDL) Great cross-platform multimedia library
- [Clay](https://github.com/nicbarker/clay) Great UI layout library.
- [rocket](https://github.com/tripleslash/rocket) Easy to use & safe signal library.