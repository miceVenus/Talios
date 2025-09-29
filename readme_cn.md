<div align="center">

(English original: see `readme_en.md`)

</div>

## Dev Log

### 9.29 及之前

完成 `boot.asm` 与 `loader.asm` 的基本雏形实现。

#### boot.asm 已实现功能

1. FAT12 的 BPB 签名与最小头部。
2. 基础 FAT12 根目录扫描实现。
3. 搜索并加载 `loader.bin` 至 0x10000。
4. 跳转执行二级加载器。

#### loader.asm 已实现功能

1. 搜索并搬运 `kernel.bin`：先临时放置于低地址，后转移至 0x0010_0000。
2. 开启 A20 地址线，扩展可访问物理内存范围。
3. 建立 GDT / 切换到 32-bit Protected Mode。
4. 继续模式切换准备进入 64-bit（IA-32e）。

### 9.30

（预留：继续记录新增内容）