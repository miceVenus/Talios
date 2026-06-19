# Talios

Talios 是一个面向 x86-64 PC 的实验性操作系统内核。它从 FAT12 软盘镜像启动，进入 IA-32e
Long Mode，初始化基础内核子系统，挂载 FAT32 磁盘镜像，并通过基于 `sysenter` 的系统调用路径运行一个很小的用户态 `init.bin`。

这个项目仍处在开发中。仓库里已经有比较完整的启动、内存、任务、设备和文件系统雏形，但不少操作表接口目前只是接好了入口，函数体还没有真正补完。

## 当前目标平台

- 目标架构：x86-64 / AMD64，内核运行在 IA-32e Long Mode。
- 启动方式：传统 BIOS 启动。
- 主要模拟器：Bochs。Makefile 里保留了 QEMU 注释，但当前因为 INT 13h 软盘读扇区问题转向 Bochs。
- 当前运行形态：默认路径只启动 BSP 单核。SMP 和 x2APIC 相关代码存在，但 `main()` 里 AP 启动循环目前等价关闭。
- 工具链：`gcc-9`、`ld`、`as`、`nasm`、`objcopy`、Bochs 以及 FAT 镜像工具。

目前没有 ARM、RISC-V 或纯 32 位保护模式目标。32 位保护模式只作为启动阶段进入 64 位长模式前的过渡步骤。

## 启动流程

1. BIOS 将 `bootloader/boot.asm` 生成的启动扇区加载到 `0x7c00`。
2. 启动扇区提供 FAT12 BPB，并在 FAT12 根目录里寻找 `LOADER.BIN`。
3. `bootloader/loader.asm` 负责加载 `KERNEL.BIN`，开启 A20，读取 BIOS E820 内存布局到 `0x7e00`，设置 VBE 显示模式，构造临时 GDT 和页表，检查 IA-32e 支持，最后跳入 64 位内核。
4. `kernel/head.S` 重新加载 64 位 GDT/IDT/TSS，切换到内核栈，安装早期页表，然后进入 `main()`。
5. `kernel/main.c` 初始化 printk、CPU 信息、内存管理、APIC/IRQ、softirq、调度器、键盘、软驱、ATA 磁盘、任务、HPET 和定时器。

## 支持的文件系统

- FAT12：用于启动镜像，让 BIOS 阶段代码能找到 `loader.bin` 和 `kernel.bin`。
- FAT32：通过 VFS 层挂载 Bochs ATA 磁盘镜像。当前已经有 superblock 初始化、路径查找、长文件名和短文件名匹配、文件打开、读、写、seek 等基础路径。

目前没有 ext2/ext4、tarfs、tmpfs、procfs 或 devfs 实现。

## 已实现或已有雏形的功能

- 启动加载：FAT12 boot sector、二级 loader、A20、E820 内存图、VBE、32 位过渡、IA-32e 过渡和内核搬运。
- 控制台输出：基于 framebuffer 的 `ColorPrintfk()`，支持基础格式化、颜色、光标维护和自旋锁保护。
- CPU 与描述符：CPUID 输出、GDT、IDT、TSS、trap gate 和异常处理。
- 中断系统：IRQ 描述符表、APIC/IOAPIC 控制器钩子、IRQ 注册和中断入口桩。
- 定时器：HPET 中断、CMOS 时间读取、`jiffies`、timer list 和 timer softirq。
- 内存管理：E820 解析、2 MiB 页分配器、zone/page 元数据、内核页表扩展、slab 风格的 `kmalloc()`/`kfree()` 和用户地址映射辅助函数。
- 任务与调度：task 结构、上下文切换、按 CPU 划分的调度队列、内核线程、`fork`、`vfork`、`execve` 和简单的 `vrun_time` 记账。
- 系统调用：`sysenter`/`sysexit` 路径，系统调用表包含 `putstring`、`open`、`close`、`read`、`write`、`lseek`、`fork`、`vfork`、`execve` 和 `brk`。
- 存储设备：软驱控制器代码、可选 DMA 模式、ATA LBA48 读写请求队列、IRQ 完成处理和 FAT32 集成。
- 输入设备：PS/2 键盘 IRQ、扫描码缓冲区、修饰键状态和内核主循环里的按键分析。
- 用户态：简单用户态运行库、系统调用包装、基于 `brk` 的小型 `malloc()`、用户态链接脚本和 demo init 程序。

## 构建和运行

构建 bootloader、内核和用户态镜像：

```sh
make all
```

在 Bochs 中运行：

```sh
make run
```

启用 Bochs GDB stub 进行调试：

```sh
make debug
```

`assmble.sh` 会通过 `sudo mount` 挂载镜像，把 `loader.bin`、`kernel.bin` 和 `init.bin` 拷贝进 FAT 镜像，并用 `fsck.vfat` 检查 boot 镜像。

## 仓库结构

- `bootloader/`：BIOS 启动扇区、FAT12 辅助代码和二级加载器。
- `kernel/`：内核主体、架构相关代码、驱动、VFS、FAT32、调度器、内存管理、异常、中断和系统调用。
- `user/`：用户态运行库、系统调用 ABI 包装、malloc、头文件和 demo init 进程。
- `makefile`：内核、bootloader 和用户态镜像的构建规则。
- `bochsrc.floppy`：Bochs 配置，使用软盘启动镜像和 FAT32 ATA 磁盘镜像。
- `assmble.sh`：镜像组装脚本。
- `init.py`：Bochs 远程 GDB 调试辅助脚本。

## 尚未实现完毕的地方

- FAT32 元数据操作大多还是空函数或半成品，包括 `create`、`mkdir`、`rmdir`、`rename`、`getattr`、`setattr`、`compare`、`hash`、`release`、`iput`、`close`、`ioctl` 和 superblock 写回。
- FAT32 读写已有基础 cluster I/O，但 cluster 链扩展、空闲空间统计、目录项创建、文件增长时的元数据同步还不完整。
- VFS 目前只有最小的全局挂载/root 模型，缺少挂载命名空间、引用计数、缓存回收、权限检查以及完整的 dentry/inode 生命周期管理。
- `do_fork()` 在代码中被标注为 incomplete。进程退出、wait、信号投递、copy-on-write 和完整子进程清理还没有实现。
- `execve()` 目前把平坦二进制加载到固定用户虚拟地址，没有 ELF loader、argv/envp、动态链接或按需分页。
- 用户态堆非常简单：`malloc()` 只是推进 `brk`，`free()` 还是空函数。
- SMP 仍是部分实现。AP 启动、IPI handler、per-CPU 初始化和调度结构都有代码，但当前 `main()` 默认没有真正拉起 AP。
- `interrupt.c` 中 IRQ 处理路径被标为未完成，嵌套中断、抢占、错误处理和控制器边界情况还需要继续加固。
- framebuffer 控制台只有基础绘制，滚屏、跨行退格、自动分辨率发现和更完整的终端行为还没完成。
- 磁盘和软驱驱动已经能支持当前镜像 I/O 实验，但 open/close/ioctl、错误恢复、缓存/缓冲区管理和设备发现仍不完整。
- 目前还没有网络栈、shell、用户/权限模型、IPC、管道、socket 或稳定的内核测试框架。


## 内存布局

启动阶段的低端物理内存布局大致如下：

```text
物理地址
0x0010_0000  +------------------------------------------------+
             | kernel.bin 最终加载地址                         |
             | loader 跳入 Long Mode 后从这里进入内核           |
0x0009_0000  +------------------------------------------------+
             | kernel 临时缓冲区 / 可用低端缓冲区               |
0x0008_0000  +------------------------------------------------+
             | FAT 表和目录项读取缓冲区                         |
0x0001_0000  +------------------------------------------------+
             | loader.bin 加载地址                              |
0x0000_9000  +------------------------------------------------+
             | 临时页表：PML4/PDPT/PD                            |
0x0000_7e00  +------------------------------------------------+
             | E820 内存表缓冲区 / 临时栈                        |
0x0000_7c00  +------------------------------------------------+
             | BIOS 加载 boot sector                            |
0x0000_0000  +------------------------------------------------+
             | BIOS 实模式区域、中断向量表等                     |
```

进入 64 位内核后的主要虚拟地址布局：

```text
虚拟地址
0xffff_8000_0300_0000  +---------------------------------------+
                       | framebuffer 映射                       |
0xffff_8000_0200_0000  +---------------------------------------+
                       | AP 启动代码拷贝区，SMP 启动时使用       |
0xffff_8000_0010_0000  +---------------------------------------+
                       | kernel 映像起始地址                     |
                       | .text / .rodata / .data / init_task / .bss |
                       | _end 之后放置内存管理结构、slab、kmalloc |
0xffff_8000_0000_0000  +---------------------------------------+
                       | PAGE_OFFSET，物理地址直接映射窗口起点   |
                       +---------------------------------------+
0x0000_7fff_ffff_ffff  | 用户地址上限 TASK_SIZE                  |
                       |                                       |
0x0000_0000_00c0_0000  +---------------------------------------+
                       | 用户 brk / heap 起点                    |
0x0000_0000_00a0_0000  +---------------------------------------+
                       | 用户栈初始地址                          |
0x0000_0000_0080_0000  +---------------------------------------+
                       | 用户程序 init.bin 加载地址              |
0x0000_0000_0000_0000  +---------------------------------------+
```

这张图描述的是当前代码里的固定布局：内核高半区通过 `PAGE_OFFSET = 0xffff800000000000` 建立直接映射，用户态 demo 还没有 ELF loader 和按需分页，因此 `execve()` 暂时把程序、栈和堆放在固定地址。