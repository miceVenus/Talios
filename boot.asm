org                0x7c00

jmp            label_start
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

Ld_File_Name:    db "LOADER  BIN"
start_boot_msg:  db "Start Boot-----"
Not_Found_Ld_File_Msg: db "Not Found loader.bin"

BaseOfStack    equ 0x7c00
BaseOfLoader   equ 0x1000
OffsetOfLoader equ 0x00

RootDirSectors          equ 14
SectorNumOfRootDirStart equ 19
FirstFATSecNum          equ 1
SectorBalance           equ 17 ; 19 - 2  

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

label_start:
    mov ax,     cs
    mov ds,     ax
    mov es,     ax
    mov ss,     ax
    mov sp,     BaseOfStack
    ; ==== clear screen

    mov ax,     0600h
    mov bx,     0700h
    mov cx,     0
    mov dx,     0184fh
    int 10h
    ; ==== set focus

    mov ax,     0200h
    mov bx,     0000h
    mov dx,     0000h
    int 10h
    ; ==== display on screnn : Start Booting

    mov ax,     1301h
    mov bx,     000fh
    mov dx,     0000h
    mov cx,     15
    push        ax
    mov ax,     ds
    mov es,     ax
    pop ax
    mov bp,     start_boot_msg
    int 10h

    jmp Loop_Travel_Root_Dir

Loop_Travel_Root_Dir:
    mov cl,     SectorNumOfRootDirStart
    mov ax,     RootDirSectors

Loop_Root_Dir_Next:
    cmp ax,     0
    jz  Loop_Root_Dir_End

    mov bx,     0x8000

    push        ax
    push        cx

    mov ax,     cx
    mov cl,     1
    call        Func_ReadSector

    pop         cx
    pop         ax

    inc cl
    dec ax
 
Loop_Travel_Entries_Name:

    mov dx,     0x10
    mov bx,     0x8000

Loop_Entries_Name_Next:
    cmp dx,     0
    jz  Loop_Entries_Name_End
    dec dx

Loop_Cmp_Entry_Name:

    push        ax
    push        cx
    push        dx

    mov si,     Ld_File_Name
    cld
    mov cx,     11

Cmp_Entry_Name_Next:
    cmp cx,     0
    jz  Cmp_Entry_Name_End
    dec cx

    mov di,     bx
    inc bx

    lodsb
    cmp al,     [es:di]
    jz  Cmp_Entry_Name_Next
    jmp Cmp_Entry_Name_Diff

Cmp_Entry_Name_Diff:

    pop         dx
    pop         cx
    pop         ax

    and bx,     0xffe0
    add bx,     0x20
    jmp Loop_Entries_Name_Next

Cmp_Entry_Name_End:
    pop         dx
    pop         cx
    pop         ax

    jmp Loop_Entries_Name_End



Loop_Entries_Name_End:
    cmp dx,     0
    jz  Loop_Root_Dir_Next
    jmp Loop_Root_Dir_End

    

Loop_Root_Dir_End:
    cmp ax,     0
    jz  Loop_Root_Dir_Fail
    jmp Loop_Root_Dir_Success

Loop_Root_Dir_Fail:
    ; ==== display on screnn : Not Found loader.bin

    mov ax,     1301h
    mov bx,     008ch
    mov dx,     0100h
    mov cx,     20
    push        ax
    mov ax,     ds
    mov es,     ax
    pop ax
    mov bp,     Not_Found_Ld_File_Msg
    int 10h
    jmp $

Loop_Root_Dir_Success:
    ; ==== es:di point to entry of loader.bin
    and di,     0xffe0
    add di,     0x1a
    mov word    ax, [es:di]

    mov dx,     BaseOfLoader
    mov es,     dx
    mov bx,     OffsetOfLoader

Loop_Get_Loader_Next:

    ; ==== get plot some dot on screen
    push        ax
    push        bx
    mov ah,     0x0e
    mov al,     '.'
    mov bl,     0x0f
    int 10h
    pop         bx
    pop         ax

    ; loading loader.bin
    mov cx,     ax
    add cx,     RootDirSectors
    add cx,     SectorBalance

    push        ax

    mov ax,     cx
    mov cl,     1

    call        Func_ReadSector

    add bx,     [BPB_BytesPerSec]

    pop         ax

    call        Func_Get_FAT_Entry

    cmp ax,     0x0fff
    jnz Loop_Get_Loader_Next
    jmp Loader_Jump

Loader_Jump:
    ; ==== jump to loader
    jmp BaseOfLoader:OffsetOfLoader


    

; ==== fill zero until whole sector

times 510 - ($ - $$) db 0
dw 0xaa55
db 0xf0
db 0xff
db 0xff