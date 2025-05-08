# Maintainer: ussur, Redchin Daniil <redchindaniil@gmail.com>

pkgname=switch-netns
pkgver=1.0.0
pkgrel=1
pkgdesc="CLI tool to change network namespaces without root access"
arch=('any')
url="https://github.com/USSURATONCACHI/switch-netns.git"
license=('MIT')
depends=('libcap')
makedepends=('base-devel' 'gengetopt')
source=("git+https://github.com/USSURATONCACHI/switch-netns.git#tag=v${pkgver}")
sha256sums=('9a99d15ec923d3ec57ec4cf21afb4fe5bf78a2b4b23f91b6d2e786e9ba1d028d')

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
