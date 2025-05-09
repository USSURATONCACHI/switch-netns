# Maintainer: ussur, Redchin Daniil <redchindaniil@gmail.com>

pkgname=switch-netns
pkgver=1.0.2
pkgrel=3
pkgdesc="CLI tool to change network namespaces without root access"
arch=('any')
url="https://github.com/USSURATONCACHI/switch-netns.git"
license=('MIT')
depends=('libcap')
makedepends=('base-devel' 'gengetopt')
source=("git+${url}#tag=v${pkgver}-${pkgrel}")
sha256sums=('4b32b1ab744934edcc570c3e039bfa9459f707677acceffaae7283197aeca812')

build() {
    cd "$srcdir/switch-netns"
    make build
}

package() {
    cd "$srcdir/switch-netns"
    install -Dm755 build-dir/switch-netns "$pkgdir/usr/bin/switch-netns"
	setcap cap_sys_admin,cap_sys_ptrace=ep "$pkgdir/usr/bin/switch-netns"
    
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
