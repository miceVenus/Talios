<div align="center">

(Chinese original: see `readme_cn.md`)

</div>


## 7. Dev Log

### Up to 9.29

Initial skeletal implementation of `boot.asm` and `loader.asm`.

`boot.asm` done:
1. Minimal FAT12 BPB + head.
2. Root directory scan.
3. Load `loader.bin` to 0x10000.
4. Jump to stage two.

`loader.asm` done:
1. Locate & stage `kernel.bin`.
2. A20 enable.
3. Build GDT / switch to 32-bit Protected Mode.
4. Begin groundwork toward 64-bit mode.

### 9.30

(Reserved for future notes.)
