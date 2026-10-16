pkgname=capture-card-relay
pkgver={{pkgversion}}
pkgrel=1
arch=('any')
pkgdesc="Displays a capture cards video and microphone, with extra scaling and volume features"
provides=('capture-card-relay')
conflicts=('capture-card-relay')
makedepends=('cmake' 'extra-cmake-modules')
optdepends=(
  'sdl3: to use the system SDL3 library'
  'sdl3_ttf: to use the system SDL3-TTF library'
  'pipewire: to enable using audio through pipewire links'
)
license=('GPL-3.0')
url="{{repo}}"
source=("{{source}}")
sha256sums=('{{hash}}')

prepare() {
  cd "CaptureCardRelay-v${pkgver}"
}

build() {
  local cmake_options=(
    -B build
    -S "CaptureCardRelay-v${pkgver}"
    -D CMAKE_BUILD_TYPE=Release
    -D CMAKE_INSTALL_PREFIX=/usr
    -D App_BINARY_NAME=capture-card-relay
    -Wno-dev
  )

  cmake "${cmake_options[@]}"
  cmake --build build
}

package() {
  DESTDIR="${pkgdir}" cmake --install build
}
