%ifndef FAT12_INC
%define FAT12_INC

BS_OEMName:      db "OsBoot  "
BPB_BytesPerSec: dw 512
BPB_SecPerClus:  db 1
BPB_RsvdSecCnt:  dw 1
BPB_NumFATs:     db 2
BPB_RootEntCnt:  dw 224
BPB_TotSec16:    dw 2880
BPB_Media:       db 0xf0
BPB_FATSz16:     dw 9
BPB_SecPerTrk:   dw 18
BPB_NumHeads:    dw 2
BPB_HiddSec:     dd 0
BPB_TotSec32:    dd 0
BS_DrvNum:       db 0
BS_Reservedl:    db 0
BS_BootSig:      db 0x29
BS_VolID:        dd 0
BS_VolLab:       db "boot loader"
BS_FileSysType:  db "FAT12"

RootDirSectors          equ 14
SectorNumOfRootDirStart equ 19
FirstFATSecNum          equ 1
SectorBalance           equ SectorNumOfRootDirStart - 2  

Odd:             db     0

Func_ReadSector:
    ; ==== read sector from disk
    ; ==== paramater ES:BX;CL(NUM);AX(LBA) need passing
    ; ==== LBA to CHS conversion formula:
    ; ==== Sector   = (LBA % SecPerTrk) + 1
    ; ==== Head     = (LBA / SecPerTrk) % NumHeads
    ; ==== Cylinder = LBA / (SecPerTrk * NumHeads)
    push        bp
    mov bp,     sp
    sub esp,    2
    mov byte    [bp - 2], cl    ; 保存要读取的扇区数
    push        bx
    
    ; 第一步：计算 Sector = (LBA % SecPerTrk) + 1
    xor dx,     dx              ; DX:AX = LBA (清空DX为除法准备)
    mov bl,     [BPB_SecPerTrk] ; BL = 18
    div bl                      ; AL = LBA / 18 (商), AH = LBA % 18 (余数)
    inc ah                      ; 扇区号从1开始计数
    mov cl,     ah              ; CL = 扇区号
    
    ; 第二步：计算 Head 和 Cylinder
    ; 此时 AL = LBA / SecPerTrk
    mov dh,     al              ; 暂存到DH
    xor ah,     ah              ; AX = LBA / SecPerTrk
    mov bl,     [BPB_NumHeads]  ; BL = 2
    div bl                      ; AL = Cylinder, AH = Head
    mov ch,     al              ; CH = 柱面号
    mov dh,     ah              ; DH = 磁头号
    
    pop bx
    mov dl,     [BS_DrvNum]     ; DL = 驱动器号
Lable_Go_One_Reading:
    mov ah,     0x02            ; AH = 02h (读扇区功能)
    mov byte    al, [bp - 2]    ; AL = 要读取的扇区数
    int 13h
    jc Lable_Go_One_Reading     ; 如果出错则重试
    add esp,    2
    pop bp
    ret

Func_Get_FAT_Entry:
    ; ==== get FAT entry
    ; ==== paramater Ah(index) need passing return in Ah
    push        es
    push        bx
    push        ax
    mov ax,     0x00
    mov es,     ax
    pop ax
    mov byte    [Odd],  0
    mov bx,     3
    mul bx
    mov bx,     2
    div bx
    cmp dx,     0
    jz  Label_Even
    mov byte    [Odd],  1
Label_Even:

    xor dx,     dx
    mov bx,     [BPB_BytesPerSec]
    div bx
    push        dx
    mov bx,     8000h
    add ax,     FirstFATSecNum
    mov cl,     2
    call        Func_ReadSector

    pop dx
    add bx,     dx
    mov ax,     [es:bx]
    cmp byte    [Odd],  1
    jnz Label_Even_2
    shr ax,     4

Label_Even_2:
    and ax,     0x0fff
    pop bx
    pop es
    ret

%endif