pkgname=capture-card-relay
pkgver=1
pkgrel=3
arch=('any')
pkgdesc="Displays capture card output"
depends=('meson' 'ninja' 'sdl3-git' 'sdl3_ttf-git' 'sdl3_image-git')
license=('GPL')
url="https://github.com/coolguy1842/CaptureCardRelay"
source=("https://github.com/coolguy1842/CaptureCardRelay/archive/refs/heads/master.zip")
sha256sums=('SKIP')

build() {
  cd $srcdir/CaptureCardRelay-master
  meson setup build
  cd build
  meson compile
}

package() {
  install -Dm 755 "${srcdir}/CaptureCardRelay-master/build/CaptureCardRelay" "$pkgdir"/usr/bin/$pkgname
  install -Dm 755 "${srcdir}/CaptureCardRelay-master/assets/capture-card-relay.desktop" "$pkgdir"/usr/share/applications/${pkgname}.desktop
  install -Dm 755 "${srcdir}/CaptureCardRelay-master/assets/capture-card-relay.png" "$pkgdir"/usr/share/icons/hicolor/64x64/apps/${pkgname}.png

  sed -i 's/Exec=CaptureCardRelay/Exec=capture-card-relay/g' "$pkgdir"/usr/share/applications/${pkgname}.desktop
}
