# Maintainer: ussur, Redchin Daniil <redchindaniil@gmail.com>

pkgname=switch-netns
pkgver=1.0.1
pkgrel=1
pkgdesc="CLI tool to change network namespaces without root access"
arch=('any')
url="https://github.com/USSURATONCACHI/switch-netns.git"
license=('MIT')
depends=('libcap')
makedepends=('base-devel' 'gengetopt')
source=("git+${url}#tag=v${pkgver}")
sha256sums=('1ffdd9eabb26143369a2df4bcd7f4271df467ddd707dfaf674e5b3b213bcefdf')

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
