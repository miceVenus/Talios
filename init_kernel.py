import gdb

# ============================================
# 内核层调试脚本 - 使用物理地址断点
# ============================================
# 关键理解：
#   - 物理加载地址: 0x100000
#   - 虚拟链接地址: 0xffff800000100000
#   - PrintkInit:   0x104d51 (物理) / 0xffff800000104d51 (虚拟)
#   - main:         0x104dfc (物理) / 0xffff800000104dfc (虚拟)
# ============================================

print("=== 连接到 Bochs gdbstub ===")
gdb.execute('target remote 127.0.0.1:1234')

# 1. 加载内核符号文件（用于显示函数名）
print("=== 加载内核符号 ===")
gdb.execute('add-symbol-file bin/system.bin 0x100000')

# 2. 设置 64 位架构（内核是 64 位）
print("=== 设置架构为 x86-64 ===")
gdb.execute('set architecture i386:x86-64')

# 3. 在内核入口设置物理地址断点
print("=== 设置断点（物理地址）===")

# head.S 入口 __start
gdb.execute('hbreak *0x100000')
print("✓ 断点 1: __start (head.S 入口) at 0x100000")

# entry_64 段（高地址映射建立后）
gdb.execute('hbreak *0x10004c')
print("✓ 断点 2: entry_64 (页表启用后) at 0x10004c")

# PrintkInit 函数（物理地址）
gdb.execute('hbreak *0x104d51')
print("✓ 断点 3: PrintkInit at 0x104d51")

# main 函数（物理地址）
gdb.execute('hbreak *0x104dfc')
print("✓ 断点 4: main at 0x104dfc")

# 6. 显示所有断点
print("\n=== 当前断点列表 ===")
gdb.execute('info breakpoints')

# 7. 继续执行（让 bootloader 自然运行到内核）
print("\n=== 继续执行，等待断点触发 ===")
print("预期行为：")
print("  1. 首先停在 0x100000 (__start)")
print("  2. 单步执行 head.S 初始化")
print("  3. 停在 0x104d51 (PrintkInit)")
print("  4. 停在 0x104dfc (main)")
print("\n提示：使用 'si' 单步执行，'c' 继续到下一个断点\n")

gdb.execute('continue')
