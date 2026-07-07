#!/usr/bin/bash
sudo umount /mnt 2>/dev/null || true
rm -f bin/fat32disk.img
dd if=/dev/zero of=bin/fat32disk.img bs=1M count=128
mkfs.vfat -F 32 -S 512 -s 1 bin/fat32disk.img
sudo chown $USER:$USER bin/fat32disk.img
chmod 664 bin/fat32disk.img