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
    ; ==== paramater ES:BX;CL(NUM);AX(LAB) need passing
    push        bp
    mov bp,     sp
    sub esp,    2
    mov byte    [bp - 2], cl
    push        bx
    mov bl,     [BPB_SecPerTrk]
    div bl      ; AX / BL -> AL = 磁道号, AH = 偏移量
    inc ah      ; count start from 1
    mov cl,     ah
    mov dh,     al
    and dh,     1
    shr al,     1
    mov ch,     al
    pop bx
    mov dl,     [BS_DrvNum]
Lable_Go_One_Reading:
    mov ah,     0x02
    mov byte    al, [bp - 2]
    int 13h
    jc Lable_Go_One_Reading
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