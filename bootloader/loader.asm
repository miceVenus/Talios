org 0x10000

jmp     _start

%include 'bootloader/fat12.inc'

%define  BaseOfStack        0x7c00
%define  BaseOfKernelAddr   0x00
%define  OffsetOfKernelFile 0x100000

%define  BaseTempOfKernelAddr   0x00
%define  OffsetTempOfKernelFile 0x7E00

%define  MemStructBufferAddr 0x7E00

%define  DirEntryNameSize   11
%define  DirEntrySize       32

DirnumPerSector  equ        512 / DirEntrySize

%macro Show_Normal_Msg_Through_Intr 3
    ; ==== deprecated  until switched into protect mode====
    ; ==== parameter: (Row)1;(MsgLen)2;(MsgAddr)3 need passing ====
    mov     ax,     1301h
    mov     bx,     000fh
    mov     dh,     %1
    mov     dl,     00
    mov     cx,     %2
    push    ax
    mov     ax,     ds
    mov     es,     ax
    pop     ax
    mov     bp,     %3
    int     10h
%endmacro

%macro Show_Warnning_Msg_Through_Intr 3
    ; ==== deprecated  until switched into protect mode====
    ; ==== parameter: (Row)1;(MsgLen)2;(MsgAddr)3 need passing ====
    mov     ax,     1301h
    mov     bx,     008ch
    mov     dh,     %1
    mov     dl,     00
    mov     cx,     %2
    push    ax
    mov     ax,     ds
    mov     es,     ax
    pop     ax
    mov     bp,     %3
    int     10h
%endmacro

section .data

start_load_msg:             db 'Start loading...', 0
kernelFileName:             db 'KERNEL  BIN', 0
Not_Found_KernelFile_Msg:   db 'Not found KERNEL.BIN', 0
Get_MemStruct_Fail_Msg:     db 'Get memory structure fail', 0
Get_MemStruct_Success_Msg:  db 'Get memory structure success', 0
StartGetMemStruct_Msg:      db 'Start getting memory structure...', 0
Fail_Set_SVGA_Mode:         db 'Fail to set SVGA Mode...', 0
Success_Set_SVGA_Mode:      db 'Success to set SVGA Mode...', 0
StartSwitchIntoProtectMode: db 'Start Switch Into Protect Mode', 0
Support_IA32E_Mode_Msg:     db 'Support IA-32E Mode', 0


DisplayPosition:    dw (80*0 + 39) * 2

GdtPtr_32:          dw GdtLen_32 - 1
                    dd GdtBase_32

GdtBase_32: dd      0x00000000, 0x00000000  ; NULL 描述符
CODE_32:    dd      0x0000ffff, 0x00cf9a00  ; 代码段（基址 0，限长 4GB）
DATA_32:    dd      0x0000ffff, 0x00cf9200  ; 数据段（基址 0，限长 4GB）

GdtLen_32   equ     $ - GdtBase_32


SelectorCode32  equ CODE_32 - GdtBase_32
SelectorData32  equ DATA_32 - GdtBase_32

GdtPtr_64:          dw GdtLen_64 - 1
                    dq GdtBase_64

GdtBase_64: dq      0x0000000000000000
CODE_64:    dq      0x0020980000000000
DATA_64:    dq      0x0000920000000000

GdtLen_64   equ     $ - GdtBase_64


SelectorCode64  equ CODE_64 - GdtBase_64
SelectorData64  equ DATA_64 - GdtBase_64



IdtPtr:             
        dw      IdtLen - 1
        dd      IdtBase_32

IdtBase_32:
        times   256     dq 0

IdtLen  equ     $ - IdtBase_32


section .s16lib

bits 16

Function_Show_Hex:
    ; === paramater (num to be displayed) 1b
    ; === return Nothing
    push    ebp
    mov     ebp,    esp

    mov     al,     [ebp + 5]

    push    edi
    push    ebx

    mov     edi,    [DisplayPosition]

    mov     dl,     al
    shr     al,     4

    mov     cx,     2

    Show_Hex:

        and     al,     0x0f

        cmp     al,     9

        ja      OverNine

        add     al,     '0'
        jmp     Show_Hex_Body

        OverNine:

        sub     al,     9
        add     al,     'A'

        Show_Hex_Body:

        mov     bx,     0xB800
        mov     gs,     bx
        mov     ah,     0x0F
        mov     [gs:edi],   ax 
        add     edi,    2
        mov     dl,     al

        loop Show_Hex

Show_Hex_Done:

    pop     ebx
    pop     edi
    pop     ebp
    ret



Function_Cmp_Entry_Name:
    ; ==== Compare Entry Name In Buffer With The Hard Coding Name ====
    ; ==== paramater (NameLen)2b;(Hard Coding Name)2b;(Buffer)2b need passing ====
    ; ==== return 0(equal); !=0(not equal) ====
    push    ebp
    push    ebx
    mov     ebp,    esp

    mov     ah,     [ebp + 10] ; Name Len
    mov     si,     [ebp + 12] ; Hard Coding Name
    mov     bx,     [ebp + 14] ; Buffer

    mov     cl,     ah

    Cmp_Entry_Name_Next:

    cmp     cl,     0
    jz     Cmp_Entry_Name_End
    dec     cl
    
    mov     di,     bx
    inc     bx

    lodsb
    cmp     byte al, [es:di]
    jz      Cmp_Entry_Name_Next
    jmp     Cmp_Entry_Name_End

    Cmp_Entry_Name_End:
    cmp    cl,     0
    jz     Cmp_Entry_Name_Equal
    jmp    Cmp_Entry_Name_NotEqual

    Cmp_Entry_Name_Equal:
        mov     ax,     0
        jmp     Cmp_Entry_Name_Done
    
    Cmp_Entry_Name_NotEqual:
        mov     ax,     1
        jmp     Cmp_Entry_Name_Done

Cmp_Entry_Name_Done:
    pop     ebx
    pop     ebp
    ret 

Function_SearchKernelFile:
    Search_In_Sector:
        ; ====  Search kernel file In Every Sector ====
        mov     ax,     SectorNumOfRootDirStart
        mov     cl,     RootDirSectors
        mov     bx,     0x8000

        call    Func_ReadSector

    Search_In_Next_Sector:

        cmp     cl,     0
        jz      Not_Found_KernelFile
        dec     cl

        Search_In_Entry:

            mov     ch,     DirnumPerSector
        
        Search_In_Next_Entry:
            cmp     ch,     0
            jz      Search_In_Next_Sector
            dec     ch

            mov     si,     kernelFileName
            mov     ax,     DirEntryNameSize

            push    cx

            push    bx
            push    si
            push    ax

            call   Function_Cmp_Entry_Name

            add     sp,     6

            pop     cx

            cmp     ax,     0
            jz      Found_KernelFile
            add     bx,     DirEntrySize 
            jmp     Search_In_Next_Entry
    
    Found_KernelFile:
        mov    ax,     [bx + 0x1a] ; Start Cluster
        ret

    Not_Found_KernelFile:

        Show_Warnning_Msg_Through_Intr 02, 20, Not_Found_KernelFile_Msg

        jmp $

Function_Load_Kernel_File:
    ; ==== parameter: (StartCluster)2b; ====
    ; ==== return: Kernel Start Addr In (AX) ====
    push    ebp
    mov     ebp,    esp
    mov     ax,     [ebp + 6] ; Start Cluster

    push    es
    push    bx
    push    fs

    push    esi
    push    edi

    mov     cx,     BaseTempOfKernelAddr
    mov     es,     cx
    mov     cx,     OffsetTempOfKernelFile
    mov     bx,     cx

    Move_Kernel_Temp_Addr:
        ; ==== Warning capacity of BX is Only 33k ====
        push    ax
        add     ax,     SectorBalance
        add     ax,     RootDirSectors

        mov     cl,     1 
        call    Func_ReadSector
        add     bx,     word [BPB_BytesPerSec]

        pop     ax

        call    Func_Get_FAT_Entry

        cmp     ax,     0x0fff
        jz      Move_Kernel_High_Addr
        jmp     Move_Kernel_Temp_Addr
    
    Move_Kernel_High_Addr:
        ; ==== Move Kernel File From Temp Addr To Final Addr ====
    add     bx,     word [BPB_BytesPerSec]
    sub     bx,     OffsetTempOfKernelFile
    mov     cx,     bx

    mov     ax,     BaseTempOfKernelAddr
    mov     es,     ax
    mov     ax,     BaseOfKernelAddr
    mov     fs,     ax

    mov     esi,    OffsetTempOfKernelFile
    mov     edi,    OffsetOfKernelFile

    Move_Kernel_High_Addr_loop:

        mov     al,     byte    [es:esi]
        mov     byte    [fs:edi],     al

        inc     esi
        inc     edi

        loop   Move_Kernel_High_Addr_loop

    ALL_Done:

        pop     edi
        pop     esi
        pop     fs
        pop     bx
        pop     es
        pop     ebp
        ret


section .segment16

bits 16

_start:
    ;   ==== Set up segment register ====
    mov     ax,     cs
    mov     ds,     ax
    mov     es,     ax
    mov     ax,     0x0
    mov     ss,     ax
    mov     sp,     BaseOfStack

Open_Addr_A20:
    in      al,     0x92
    or      al,     00000010b
    out     0x92,   al

    ;   ==== Enter Big Real Mode

    cli

    db      0x66
    lgdt    [GdtPtr_32]

    mov     eax,    cr0
    or      eax,    1
    mov     cr0,    eax

    mov     ax,     SelectorData32
    mov     fs,     ax
    mov     eax,    cr0
    and     al,     11111110b
    mov     cr0,    eax

    sti

;   ==== Start loading Message

    Show_Normal_Msg_Through_Intr    02, 16, start_load_msg

    Call    Function_SearchKernelFile

    push    ax

    Call    Function_Load_Kernel_File

    add     esp,     2

    Kill_Floppy_Motor:
        push    dx
        mov     dx,     0x3F2
        mov     al,     0
        out     dx,     al
        pop     dx
    
    Show_Normal_Msg_Through_Intr    03, 33, StartGetMemStruct_Msg

    mov         ebx,     0
    mov         ax,      0
    mov         es,      ax
    mov         edi,     MemStructBufferAddr

Get_MemStruct:

    mov         eax,     0x0E820
    mov         ecx,     20
    mov         edx,     0x534D4150  ; 'SMAP'
    int         15h
    jc          Get_MemStruct_Fail
    add         edi,     20
    cmp         ebx,     0
    jne         Get_MemStruct
    jmp         Get_MemStruct_Done

    Get_MemStruct_Fail:
    Show_Warnning_Msg_Through_Intr  04, 25, Not_Found_KernelFile_Msg
    jmp     $

    Get_MemStruct_Done:
    Show_Normal_Msg_Through_Intr    04, 28, Get_MemStruct_Success_Msg

Set_SVGA_Mode:

    mov ax,     0x4F02
    mov bx,     0x4180
    int 10h

    cmp ax,     0x004F
    jne Setting_SVGA_fail
    jmp Setting_SVGA_Success

    Setting_SVGA_fail:
    Show_Warnning_Msg_Through_Intr  05, 24, Fail_Set_SVGA_Mode

    Setting_SVGA_Success:
    Show_Normal_Msg_Through_Intr    05, 27, Success_Set_SVGA_Mode

Switch_Into_Protect_Mode:

    cli

    db      0x66
    lgdt    [GdtPtr_32]

    mov     eax,    cr0
    or      eax,    1b
    mov     cr0,    eax

    jmp     dword   SelectorCode32:Temp_Protect_Mode_Entry

section .s32lib

bits 32


Function_Is_Support_IA32E_Mode:
    ; ==== Is Support CPUID ====
    ; ==== return: (EAX) 0(not support); 1(support) ====
    pushfd
    pushfd
    xor     dword [esp], 0x00200000
    popfd
    pushfd
    pop     eax
    xor     eax, [esp]
    popfd
    and     eax, 0x00200000
    cmp     eax, 0
    jz      No_IA32E_Mode  ; === could not be inverted, not support CPUID ====

    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000001
    jb      No_IA32E_Mode  ; === if max ext < 0x80000001
    mov     eax, 0x80000001
    cpuid

    test    edx, 0x20000000  ; === EDX bit 29 (LM bit) ====
    jz      No_IA32E_Mode    ;

    mov     eax, 1
    ret

No_IA32E_Mode:
    mov     eax, 0
    ret


section .segment32

bits 32

Temp_Protect_Mode_Entry:
    ; ==== a temp address to switch into IA-32E Mode ====
    mov     ax,     SelectorData32
    mov     ds,     ax
    mov     es,     ax
    mov     fs,     ax
    mov     ss,     ax
    mov     esp,    0x7E00

    call    Function_Is_Support_IA32E_Mode

    cmp     eax,    1
    je      Has_Long_Mode

    jmp     $

Has_Long_Mode:
    ; ==== Set up for IA-32E Mode ====

    ; ==== Page Table ====

    mov     dword   [0x90000],  0x91007
    mov     dword   [0x90800],  0x91007

    mov     dword   [0x91000],  0x92007

    mov     dword   [0x92000],  0x000083
    mov     dword   [0x92008],  0x200083
    mov     dword   [0x92010],  0x400083
    mov     dword   [0x92018],  0x600083
    mov     dword   [0x92020],  0x800083
    mov     dword   [0x92028],  0xa00083

    ; ==== Load GDTR For IA-32E Mode ====
    db      0x66
    lgdt    [GdtPtr_64]
    mov     ax,     SelectorData64
    mov     ds,     ax
    mov     es,     ax
    mov     fs,     ax
    mov     gs,     ax
    mov     ss,     ax

    mov     esp,    0x7e00

    mov     eax,    cr4
    bts     eax,    5           ; Set PAE bit (bit 5, not bit 4!)
    mov     cr4,    eax

    mov     eax,    0x90000  ; PML4 table address
    mov     cr3,    eax

    mov     ecx,    0xC0000080
    rdmsr
    or      eax,    0x00000100  ; Set LME bit
    wrmsr

    mov     eax,    cr0
    bts     eax,    0           ; Set PE bit
    bts     eax,    31          ; Set PG bit
    mov     cr0,    eax

    jmp     SelectorCode64:OffsetOfKernelFile



