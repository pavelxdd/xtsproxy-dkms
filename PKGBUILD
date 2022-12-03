_pkgbase=xtsproxy
pkgname=xtsproxy-dkms
pkgver=1
pkgrel=1
pkgdesc="Crypto API AES-XTS synchronous driver (DKMS)"
arch=('x86_64')
license=('GPL2')
depends=('dkms')
conflicts=("${_pkgbase}")
install="${pkgname}.install"
source=("xtsproxy.c"
        "Makefile"
        "dkms.conf")
sha256sums=('e21278f3ac8a609ca6709b180d44c67707d5ad86d8e86dcf9d7f960d8353f6de'
            '91bcdb3a04fbb4b31dfff9418c8394170d552ae446fde8746924c095f0c10915'
            'f65cb2d7c2077049b46a8315880596d1ab178bf366fe544a2b234fd5159b7f61')

package() {
    install -dm 755 "${pkgdir}/usr/src/${pkgname}"
    install -m 644 -T xtsproxy.c "${pkgdir}/usr/src/${pkgname}/xtsproxy.c"
    install -m 644 -T Makefile "${pkgdir}/usr/src/${pkgname}/Makefile"
    install -m 644 -T dkms.conf "${pkgdir}/usr/src/${pkgname}/dkms.conf"
}
