#!/usr/bin/bash
nasm boot.asm -o boot.bin && \
dd if=boot.bin of=boot.img bs=512 count=1 conv=notrunc && \
nasm loader.asm -o loader.bin && \
mount boot.img /mnt -t vfat -o loop && \
cp loader.bin /mnt/loader.bin && \
cp kernel.bin /mnt/kernel.bin && \
sync && \
umount /mnt && \
fsck.vfat -vn boot.img | head -n 5 && \
echo "Done"