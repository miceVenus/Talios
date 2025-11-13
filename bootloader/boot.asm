
;|-------In Real Mode---|
;|----------------------|
;|	100000 ~ END	    |
;|	   KERNEL	        |
;|----------------------|
;|	E0000 ~ 100000	    |
;| Extended System BIOS |
;|----------------------|
;|	C0000 ~ Dffff	    |
;| ROM Expansion Area   |
;|----------------------|
;|	A0000 ~ bffff	    |
;|   Legacy Video Area  |
;|----------------------|
;|	9f000 ~ A0000	    |
;|	 BIOS reserve	    |
;|----------------------|
;|	90000 ~ 9f000	    |
;|	 kernel tmpbuf	    |
;|----------------------|
;|	10000 ~ 90000	    |
;|	   LOADER	        |
;|----------------------|
;|	8000 ~ 10000	    |
;|	  VBE info	        |
;|----------------------|
;|	7e00 ~ 8000	        |
;|	  mem info	        |
;|----------------------|
;|	7c00 ~ 7e00	        |
;|	 MBR (BOOT)	        |
;|----------------------|
;|	0000 ~ 7c00	        |
;|	 BIOS Code	        |
;|----------------------|

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
FirstFATSecNum          equ 1
SectorBalance           equ 17 ; 19 - 2
SectorNumOfRootDirStart equ 19  

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
    
    xor dx,     dx              ; DX:AX = LBA 
    mov bl,     [BPB_SecPerTrk] ; BL = 18
    div bl                      ; AL = LBA / 18 , AH = LBA % 18
    inc ah                      ; count start from 1
    mov cl,     ah              ; CL
    
    mov dh,     al             
    xor ah,     ah              ; AX = LBA / SecPerTrk
    mov bl,     [BPB_NumHeads]  ; BL = 2
    div bl                      ; AL = Cylinder, AH = Head
    mov ch,     al              ; CH 
    mov dh,     ah              ; DH
    
    pop bx
    mov dl,     [BS_DrvNum]     ; DL

    Go_On_Reading:
    mov ah,     0x02            ; AH = 02h 
    mov byte    al, [bp - 2]    ; AL
    int 13h
    jc Go_On_Reading      
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
    
    ; 保存 BIOS 传入的启动驱动器号
    mov [BS_DrvNum], dl
    
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

Loop_Travel_Root_Dir:
    mov ax,     SectorNumOfRootDirStart
    mov cx,     RootDirSectors

    mov bx,     0x8000
    call        Func_ReadSector

    Loop_Travel_Entries_Name:

        mov dx,     [BPB_RootEntCnt]

        Loop_Entries_Name_Next:
        cmp dx,     0
        jz  Loop_Entries_Name_End
        dec dx

        Loop_Cmp_Entry_Name:
            mov si,     Ld_File_Name
            mov cx,     11
            cld

            Cmp_Entry_Name_Next:
            cmp cx,     0
            jz  Loop_Entries_Name_End
            dec cx

            mov di,     bx
            inc bx

            lodsb
            cmp al,     [es:di]
            jz  Cmp_Entry_Name_Next
            jmp Cmp_Entry_Name_Diff

        Cmp_Entry_Name_Diff:
        and bx,     0xffe0
        add bx,     0x20
        jmp Loop_Entries_Name_Next

        Loop_Entries_Name_End:
        cmp dx,     0
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