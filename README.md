# Capture Card Relay
While this is meant for viewing and listening to a capture cards output, it is, essentially, a webcam viewer and a microphone relay.

#### Tested on NixOS, ArchLinux, Windows
### Controls

| Key | Action                        |
| --- | ----------------------------- |
| Left Arrow    | Cycle Camera        |
| Right Arrow   | Cycle Microphone    |
| Up/Down Arrow | Up/Lower Volume     |
| O             | Open Settings Menu  |

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
- B: Add `inputs.capturecardrelay.nixosModules.<hostPlatform>.default` to nixosSystem modules, and configure with `programs.CaptureCardRelay`.

### Building
```sh
git clone https://github.com/coolguy1842/CaptureCardRelay
cd CaptureCardRelay

meson setup build
meson compile -C build

./build/CaptureCardRelay
```

### TODO:
- Add VSync & Frame Limiter

## Thanks To
- [Clay](https://github.com/nicbarker/clay) Great UI layout library.
- [rocket](https://github.com/tripleslash/rocket) Easy to use & safe signal library.