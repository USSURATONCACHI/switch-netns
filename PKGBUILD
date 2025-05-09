# Maintainer: ussur, Redchin Daniil <redchindaniil@gmail.com>

pkgname=switch-netns
pkgver=1.0.2
pkgrel=1
pkgdesc="CLI tool to change network namespaces without root access"
arch=('any')
url="https://github.com/USSURATONCACHI/switch-netns.git"
license=('MIT')
depends=('libcap')
makedepends=('base-devel' 'gengetopt')
source=("git+${url}#tag=v${pkgver}")
sha256sums=('8af27fb42f4df02e848788e35c48649a64ce6f0aa0c7581be3d533fe063be9af')

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
