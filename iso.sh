#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/xanaduos.kernel isodir/boot/xanaduos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "xanaduos" {
	multiboot /boot/xanaduos.kernel
}
EOF
grub-mkrescue -o xanaduos.iso isodir
