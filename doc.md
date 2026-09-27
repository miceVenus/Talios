## unkonwn instruction:

    sysexit/sysenter: 快速系统调用指令对， exit from ring 0-> ring3 enter from ring 3 -> ring 0 执行无压栈操作 直接读取rcx作为 栈指针寄存器 rdx作为 地址寄存器 在一切操作开始前都会关中断

    sysenter: 寄存器加载（依赖 MSRs）：
    从三个特殊的 MSR (Model Specific Registers) 中读取预设值：
    CS (代码段)：加载自 MSR_IA32_SYSENTER_CS (0x174)
    SS (栈段)：设置为 SYSENTER_CS + 8（硬件强行规定）
    RIP (入口地址)：加载自 MSR_IA32_SYSENTER_EIP (0x176)
    RSP (内核栈指针)：加载自 MSR_IA32_SYSENTER_ESP (0x175)

    sysexit: 寄存器恢复（硬件硬编码逻辑）：
    RIP (返回地址)：直接从 RDX 寄存器加载到 RIP。
    RSP (用户栈指针)：直接从 RCX 寄存器加载到 RSP。
    CS (代码段)：设置为 SYSENTER_CS + 16（如果是 32 位模式）或特定偏移 SYSENTER_CS + 32。
    SS (栈段)：设置为 SYSENTER_CS + 40。

    lock：

    锁定北桥/系统总线（老式 CPU）：确保指令操作期间没有其他设备能访问总线。
    MESI 缓存一致性协议（现代 CPU）：通过锁住对应的缓存行（Cache Line），使得在 RMW 期间，其他核心无法读取或写入该缓存行，从而保证了这指令操作的绝对不可分割性。
    lock语义中必须用于修饰使用内存作为操作对象的指令

    movq (Move Quadword)
    操作数宽度：64 位（8 字节）
    用途：通用的 64 位数据移动指令
    语法示例：movq $0x1234, %rax 或 movq (%rsi), %rdi
    影响标志位：不影响任何标志位

    movsq (Move String Quadword)
    操作数宽度：64 位（8 字节）
    用途：字符串/块操作指令，配合 rep 前缀使用
    隐式操作数：
    源地址由 %rsi 指定，目标地址由 %rdi 指定
    执行后 %rsi 和 %rdi 会自动增加或减少 8 字节（由 DF 标志决定）
    常见用法：rep movsq 用于批量拷贝内存
    影响标志位：不直接影响标志，但 rep 前缀会检查 %rcx 计数


    在 GCC 内联汇编的约束条件（Constraint）中，=& 是两个特殊标识符的组合，各自有明确的作用：

    = (Output, Write-Only)
    表示这是一个输出操作数（从汇编往 C 赋值）
    = 意味着该操作数是只写的——输入值会被忽略，汇编指令只负责往里面写新值
    & (Early Clobber)
    表示该操作数会在指令执行前就被修改（不等待其他操作数读取完毕）
    防止编译器将此操作数与输入操作数共用同一个寄存器

## unkonwn hardware mechanism

    在 x86-64体系结构下，TSS（Task State Segment，任务状态段）不再用于保存通用的执行上下文，而是转变为一个专门用于存储特定内核栈指针的配置表。

    特权级切换时的栈同步 (Privilege Level Switching)
    这是 TSS 最核心的作用。当 CPU 从用户态（Ring 3）进入内核态（Ring 0）时（例如触发中断、异常或执行系统调用），硬件必须自动切换堆栈以确保内核运行环境的安全。
    RSP0 字段：TSS 存储了 RSP0 指针。当发生特权级提升（3 → 0）时，CPU 会自动从当前 CPU 的 TSS 中加载该值到 RSP 寄存器。RSP1 和 RSP2 字段是系统未启用字段

    中断栈表机制 (IST, Interrupt Stack Table)

    TSS 提供了 7 个独立的栈指针槽位（IST1 - IST7）。
    通过在 IDT（中断描述符表）中指定某个中断对应的 IST 索引（如索引 1），CPU 在处理该中断时会无条件强制切换到对应的 IST 栈地址。
    这解决了“内核栈溢出”或“双重故障（Double Fault）”等极端情况下的处理问题（因为此时原有的内核栈可能已经崩坏，无法正常压栈）。。


    SMP 模拟器行为记录 (Bochs)

    架构类型：单线程指令级解释。
    性能特征：模拟 CPU 数量增加会导致单核有效 IPS 按比例下降；宿主机仅表现为单核满载。
    开发影响：Bochs 无法模拟真实的硬件并行时序，但其串行交替执行特性有助于捕获逻辑上的死锁（Deadlock）和原子操作的基本错误。

    bochs 硬盘模拟 

    ata1-master: type=disk, path="bin/fat32disk.img",mode=flat,cylinders=512,heads=16,spt=32
    bochs内部似乎不会按照CHS进行寻道 因此只需要保证chs对应的扇区数一致即可
    ata1 Advanced Technology Attacgment 通道1
    IDE  integrated drive electronics   总称可指代ata


    C语言ABI red ZONE：

    在 x86-64 的 C 语言函数调用约定中，栈顶指针 %rsp 以下 128 字节区域被称为“红区”。编译器被允许在这个区域中存储一些临时局部变量或其他数据，同时不需要去减去 %rsp 指针来分配空间。这在用户态程序中是安全的，因为操作系统向用户态投递信号时会主动避开这 128 字节。

    但在内核态开发中，情况完全不同：当硬件中断（如定时器中断等）异步发生时，CPU 硬件的机制是在当前执行现场的 %rsp 处直接进行压栈操作（它会将 ss、rsp、rflags、cs、rip 甚至错误码直接推入堆栈）。这就会无情地把编译器原先寄存在“红区”里的局部变量给覆盖掉掉，因此产生了局部变量被置 0（被 pushq $0x00 覆盖或被异常帧参数覆盖）的 bug。

    解决方法：
    给 GCC 传递的 CFLAGS 中加入 -mno-red-zone 参数。

    丢失唤醒（Lost Wakeup）问题
    由于任务提交（submit 或者发送磁盘读写指令）到把自己设置为不可中断（睡眠等待 IO 完成）并执行调度的过程并不是原子操作，如果磁盘响应速度非常快（或缓存命中），在进程把自己挂起之前，硬件中断可能就已经触发，end_request（其实也是中断处理程序调用的）已经到来并将进程唤醒（设置回 RUNNING）。然后进程继续往下执行，把自己强行改成了 TASK_UNINTERRUPTABLE 并去睡眠，这就导致了它永远挂起，因为不会再有中断来唤醒它。

    解决方法
    将TASK_UNINTERRUPTABLE的设置提前 并且在提交过程中关闭中断


enum IOAPIC_TRIGGER_M{
    IOAPIC_TRIGGER_EDGE = 0,
    IOAPIC_TRIGGER_LEVEL,
};

触发模式的配置错误导致系统启动速度变得极其缓慢

## TODO

还没有实现FAT32的inode API

还没完全实现各种硬件驱动 