pkgname=capture-card-relay
pkgver=1
pkgrel=1
arch=('any')
pkgdesc="Displays capture card output"
depends=('meson' 'ninja' 'sdl3-git' 'sdl3_ttf-git' 'sdl3_image-git')
license=('GPL')
url="https://github.com/coolguy1842/CaptureCardRelay"
source=("src")
sha256sums=('SKIP')

build() {
  meson setup build
  cd build
  meson compile
}

package() {
  install -d "${pkgdir}"/usr/bin
  install -m 755 "${srcdir}/build/CaptureCardRelay" "$pkgdir"/usr/bin/$pkgname
}
