jmp         _start    

.section    .data

.global     GdtTable


; Global Descriptor Table (GDT)

GdtPtr:

    dw GdtLen - 1
    dq GdtTable

GdtTable:    
    dq      0x0000000000000000  ; NULL Offset 0
    dq      0x0020980000000000  ; Kernel Code Segment Offset 0x8
    dq      0x0000920000000000  ; Kernel Data Segment Offset 0x10
    dq      0x0020f80000000000  ; User Code Segment Offset 0x18
    dq      0x0000f20000000000  ; User Data Segment Offset 0x20
    dq      0x00cf9a000000ffff  ; Kernel Code Segment 32-bit Offset 0x28
    dq      0x00cf92000000ffff  ; Kernel Data Segment 32-bit Offset 0x30
    times   10 dq 0             ; TSS Offset 0x38


GdtLen   equ     $ - GdtTable


SelectorCode64  equ CODE_64 - GdtBase_64
SelectorData64  equ DATA_64 - GdtBase_64


; Interrupt Descriptor Table (IDT)

.global     IdtTable

IdtPtr:             
    
    dw      IdtLen - 1
    dq      IdtTable

IdtTable:
    
    times   512 dq 0

IdtLen:
    
    equ     $ - IdtTable


; Task State Segment Table (TSS)

.global     TssTable

TssPtr:

    dw      TssLen - 1
    dq      TssTable

TssTable:
    
    times   10 dq 0

TssLen
    
    equ     $ - TssTable

_start: