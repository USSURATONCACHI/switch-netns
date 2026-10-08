# Maintainer: ussur, Redchin Daniil <redchindaniil@gmail.com>

pkgname=switch-netns2
pkgver=2.0.0
pkgrel=1
pkgdesc="CLI tool to run commands in network namespaces without root access"
arch=('x86_64')
url="https://github.com/USSURATONCACHI/switch-netns"
license=('MIT')
depends=('libcap')
source=('main.c' 'assert.h' 'debug.h' 'misc.h' 'panic.h' 'LICENSE')
sha256sums=('SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP')

build() {
    cc -O3 $CFLAGS $LDFLAGS -o switch-netns2 main.c -lcap
}

package() {
    install -Dm755 switch-netns2 "$pkgdir/usr/bin/switch-netns2"
    setcap cap_sys_admin=ep "$pkgdir/usr/bin/switch-netns2"

    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
