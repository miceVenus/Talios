#!/usr/bin/bash
echo "\n==== BUILD BOOT.IMG ====\n"
dd if=bin/boot.bin of=bin/boot.img bs=512 count=1 conv=notrunc && \
sudo mount bin/boot.img /mnt -t vfat -o loop && \
echo "\n==== LOAD LOADER AND KERNEL ====\n" && \
sudo cp bin/loader.bin /mnt/loader.bin && \
sudo cp bin/kernel.bin /mnt/kernel.bin && \
sudo cp bin/init.bin /mnt/init.bin && \
sudo sync && \
sudo umount /mnt && \
sudo mount bin/fat32disk.img /mnt -t vfat -o loop && \
sudo cp bin/init.bin /mnt/init.bin && \
sudo sync && \
sudo umount /mnt && \
echo "\n==== FILE SYSTEM CHECK ====\n" && \
fsck.vfat -vn bin/boot.img | head -n 5 && \
echo "==== Done ===="