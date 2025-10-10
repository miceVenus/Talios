import gdb
import re
import os

R = {}
DISASM = {}
DISASM_TARGET = "disboot"
SHOW_DISASM = True

if SHOW_DISASM:
    os.system(f"ndisasm -o 0x7c00 bin/{DISASM_TARGET[3:]}.bin > {DISASM_TARGET}.asm")
    with open(DISASM_TARGET + ".asm", 'r') as f:
        file_lines = f.readlines()
        for line in file_lines:
            addr , _, _,  asm = line.split(" ", maxsplit=3)
            try:
                DISASM[int(addr, 16)] = asm.strip()
            except ValueError:
                pass


    
def stop_handler(event):
    if isinstance(event, gdb.StopEvent):
        regs = [
            line for line in 
                gdb.execute('info registers',
                            to_string=True).
                            strip().split('\n')
        ]
        for line in regs:
            parts = line.split()
            key = parts[0]

            if m := re.search(r'(\[.*?\])', line):
                val = m.group(1)
            else:
                val = parts[1]

            if key in R and R[key] != val:
                print(key, R[key], '->', val)
            R[key] = val

        print(f"rip : {R['rip']}")
        print(f"asm : {DISASM.get(int(R['rip'], 16), '???')}")
    

gdb.events.stop.connect(stop_handler)

gdb.execute('target remote 127.0.0.1:1234')
# gdb.execute('symbol-file bin/system')
# gdb.execute('directory /home/bbm/workspace/System Dev/Talios/kernel')
gdb.execute('set disassembly-flavor intel')
gdb.execute('set architecture i386:x86-64')
# User program entry
gdb.execute('b *0x7c00')
gdb.execute('b *0x7C9B')
gdb.execute('b *0x7D37')


gdb.execute('continue')
