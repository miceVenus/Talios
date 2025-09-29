00000000  E90602            jmp 0x209
00000003  4F                dec di
00000004  7342              jnc 0x48
00000006  6F                outsw
00000007  6F                outsw
00000008  7420              jz 0x2a
0000000A  2000              and [bx+si],al
0000000C  0201              add al,[bx+di]
0000000E  0100              add [bx+si],ax
00000010  02E0              add ah,al
00000012  00400B            add [bx+si+0xb],al
00000015  F00900            lock or [bx+si],ax
00000018  1200              adc al,[bx+si]
0000001A  0200              add al,[bx+si]
0000001C  0000              add [bx+si],al
0000001E  0000              add [bx+si],al
00000020  0000              add [bx+si],al
00000022  0000              add [bx+si],al
00000024  0000              add [bx+si],al
00000026  2900              sub [bx+si],ax
00000028  0000              add [bx+si],al
0000002A  00626F            add [bp+si+0x6f],ah
0000002D  6F                outsw
0000002E  7420              jz 0x50
00000030  6C                insb
00000031  6F                outsw
00000032  61                popa
00000033  64657246          gs jc 0x7d
00000037  41                inc cx
00000038  54                push sp
00000039  3132              xor [bp+si],si
0000003B  005589            add [di-0x77],dl
0000003E  E566              in ax,0x66
00000040  83EC02            sub sp,byte +0x2
00000043  884EFE            mov [bp-0x2],cl
00000046  53                push bx
00000047  8A1E1800          mov bl,[0x18]
0000004B  F6F3              div bl
0000004D  FEC4              inc ah
0000004F  88E1              mov cl,ah
00000051  88C6              mov dh,al
00000053  80E601            and dh,0x1
00000056  D0E8              shr al,1
00000058  88C5              mov ch,al
0000005A  5B                pop bx
0000005B  8A162400          mov dl,[0x24]
0000005F  B402              mov ah,0x2
00000061  8A46FE            mov al,[bp-0x2]
00000064  CD13              int 0x13
00000066  72F7              jc 0x5f
00000068  6683C402          add esp,byte +0x2
0000006C  5D                pop bp
0000006D  C3                ret
0000006E  06                push es
0000006F  53                push bx
00000070  50                push ax
00000071  B80000            mov ax,0x0
00000074  8EC0              mov es,ax
00000076  58                pop ax
00000077  C6063B0000        mov byte [0x3b],0x0
0000007C  BB0300            mov bx,0x3
0000007F  F7E3              mul bx
00000081  BB0200            mov bx,0x2
00000084  F7F3              div bx
00000086  83FA00            cmp dx,byte +0x0
00000089  7405              jz 0x90
0000008B  C6063B0001        mov byte [0x3b],0x1
00000090  31D2              xor dx,dx
00000092  8B1E0B00          mov bx,[0xb]
00000096  F7F3              div bx
00000098  52                push dx
00000099  BB0080            mov bx,0x8000
0000009C  83C001            add ax,byte +0x1
0000009F  B102              mov cl,0x2
000000A1  E898FF            call 0x3c
000000A4  5A                pop dx
000000A5  01D3              add bx,dx
000000A7  268B07            mov ax,[es:bx]
000000AA  803E3B0001        cmp byte [0x3b],0x1
000000AF  7503              jnz 0xb4
000000B1  C1E804            shr ax,byte 0x4
000000B4  25FF0F            and ax,0xfff
000000B7  5B                pop bx
000000B8  07                pop es
000000B9  C3                ret
000000BA  0000              add [bx+si],al
000000BC  53                push bx
000000BD  7461              jz 0x120
000000BF  7274              jc 0x135
000000C1  206C6F            and [si+0x6f],ch
000000C4  61                popa
000000C5  64696E672E2E      imul bp,[fs:bp+0x67],word 0x2e2e
000000CB  2E004B45          add [cs:bp+di+0x45],cl
000000CF  52                push dx
000000D0  4E                dec si
000000D1  45                inc bp
000000D2  4C                dec sp
000000D3  2020              and [bx+si],ah
000000D5  42                inc dx
000000D6  49                dec cx
000000D7  4E                dec si
000000D8  004E6F            add [bp+0x6f],cl
000000DB  7420              jz 0xfd
000000DD  666F              outsd
000000DF  756E              jnz 0x14f
000000E1  64204B45          and [fs:bp+di+0x45],cl
000000E5  52                push dx
000000E6  4E                dec si
000000E7  45                inc bp
000000E8  4C                dec sp
000000E9  2E42              cs inc dx
000000EB  49                dec cx
000000EC  4E                dec si
000000ED  0017              add [bx],dl
000000EF  00F4              add ah,dh
000000F1  0001              add [bx+di],al
000000F3  0000              add [bx+si],al
000000F5  0000              add [bx+si],al
000000F7  0000              add [bx+si],al
000000F9  0000              add [bx+si],al
000000FB  00FF              add bh,bh
000000FD  FF00              inc word [bx+si]
000000FF  0000              add [bx+si],al
00000101  9ACF00FFFF        call 0xffff:0xcf
00000106  0000              add [bx+si],al
00000108  0092CF00          add [bp+si+0xcf],dl
0000010C  6655              push ebp
0000010E  6653              push ebx
00000110  6689E5            mov ebp,esp
00000113  678A650A          mov ah,[ebp+0xa]
00000117  678B750C          mov si,[ebp+0xc]
0000011B  678B5D0E          mov bx,[ebp+0xe]
0000011F  88E1              mov cl,ah
00000121  80F900            cmp cl,0x0
00000124  740D              jz 0x133
00000126  FEC9              dec cl
00000128  89DF              mov di,bx
0000012A  43                inc bx
0000012B  AC                lodsb
0000012C  263A05            cmp al,[es:di]
0000012F  74F0              jz 0x121
00000131  EB00              jmp short 0x133
00000133  80F900            cmp cl,0x0
00000136  7402              jz 0x13a
00000138  EB05              jmp short 0x13f
0000013A  B80000            mov ax,0x0
0000013D  EB05              jmp short 0x144
0000013F  B80100            mov ax,0x1
00000142  EB00              jmp short 0x144
00000144  665B              pop ebx
00000146  665D              pop ebp
00000148  C3                ret
00000149  B81300            mov ax,0x13
0000014C  B10E              mov cl,0xe
0000014E  BB0080            mov bx,0x8000
00000151  E8E8FE            call 0x3c
00000154  80F900            cmp cl,0x0
00000157  742A              jz 0x183
00000159  FEC9              dec cl
0000015B  B510              mov ch,0x10
0000015D  80FD00            cmp ch,0x0
00000160  74F2              jz 0x154
00000162  FECD              dec ch
00000164  BECD00            mov si,0xcd
00000167  B80B00            mov ax,0xb
0000016A  51                push cx
0000016B  53                push bx
0000016C  56                push si
0000016D  50                push ax
0000016E  E89BFF            call 0x10c
00000171  83C406            add sp,byte +0x6
00000174  59                pop cx
00000175  83F800            cmp ax,byte +0x0
00000178  7405              jz 0x17f
0000017A  83C320            add bx,byte +0x20
0000017D  EBDE              jmp short 0x15d
0000017F  8B471A            mov ax,[bx+0x1a]
00000182  C3                ret
00000183  B80113            mov ax,0x1301
00000186  BB8C00            mov bx,0x8c
00000189  BA0002            mov dx,0x200
0000018C  B91400            mov cx,0x14
0000018F  50                push ax
00000190  8CD8              mov ax,ds
00000192  8EC0              mov es,ax
00000194  58                pop ax
00000195  BDD900            mov bp,0xd9
00000198  CD10              int 0x10
0000019A  EBFE              jmp short 0x19a
0000019C  6655              push ebp
0000019E  6689E5            mov ebp,esp
000001A1  678B4506          mov ax,[ebp+0x6]
000001A5  06                push es
000001A6  53                push bx
000001A7  0FA0              push fs
000001A9  6656              push esi
000001AB  6657              push edi
000001AD  B90000            mov cx,0x0
000001B0  8EC1              mov es,cx
000001B2  B9007E            mov cx,0x7e00
000001B5  89CB              mov bx,cx
000001B7  50                push ax
000001B8  83C011            add ax,byte +0x11
000001BB  83C00E            add ax,byte +0xe
000001BE  B101              mov cl,0x1
000001C0  E879FE            call 0x3c
000001C3  031E0B00          add bx,[0xb]
000001C7  58                pop ax
000001C8  E8A3FE            call 0x6e
000001CB  3DFF0F            cmp ax,0xfff
000001CE  7402              jz 0x1d2
000001D0  EBE5              jmp short 0x1b7
000001D2  031E0B00          add bx,[0xb]
000001D6  81EB007E          sub bx,0x7e00
000001DA  89D9              mov cx,bx
000001DC  B80000            mov ax,0x0
000001DF  8EC0              mov es,ax
000001E1  B80000            mov ax,0x0
000001E4  8EE0              mov fs,ax
000001E6  66BE007E0000      mov esi,0x7e00
000001EC  66BF00001000      mov edi,0x100000
000001F2  26678A06          mov al,[es:esi]
000001F6  64678807          mov [fs:edi],al
000001FA  6646              inc esi
000001FC  6647              inc edi
000001FE  E2F2              loop 0x1f2
00000200  665F              pop edi
00000202  665E              pop esi
00000204  5B                pop bx
00000205  07                pop es
00000206  665D              pop ebp
00000208  C3                ret
00000209  8CC8              mov ax,cs
0000020B  8ED8              mov ds,ax
0000020D  8EC0              mov es,ax
0000020F  B80000            mov ax,0x0
00000212  8ED0              mov ss,ax
00000214  BC007C            mov sp,0x7c00
00000217  B80113            mov ax,0x1301
0000021A  BB0F00            mov bx,0xf
0000021D  BA0002            mov dx,0x200
00000220  B91000            mov cx,0x10
00000223  50                push ax
00000224  8CD8              mov ax,ds
00000226  8EC0              mov es,ax
00000228  58                pop ax
00000229  BDBC00            mov bp,0xbc
0000022C  CD10              int 0x10
0000022E  E492              in al,0x92
00000230  0C02              or al,0x2
00000232  E692              out 0x92,al
00000234  FA                cli
00000235  660F0116EE00      o32 lgdt [0xee]
0000023B  0F20C0            mov eax,cr0
0000023E  6683C801          or eax,byte +0x1
00000242  0F22C0            mov cr0,eax
00000245  B81000            mov ax,0x10
00000248  8EE0              mov fs,ax
0000024A  0F20C0            mov eax,cr0
0000024D  24FE              and al,0xfe
0000024F  0F22C0            mov cr0,eax
00000252  FB                sti
00000253  E8F3FE            call 0x149
00000256  50                push ax
00000257  E842FF            call 0x19c
0000025A  6683C402          add esp,byte +0x2
0000025E  B800B8            mov ax,0xb800
00000261  8EE8              mov gs,ax
00000263  B40F              mov ah,0xf
00000265  B044              mov al,0x44
00000267  65A34E00          mov [gs:0x4e],ax
0000026B  52                push dx
0000026C  BAF203            mov dx,0x3f2
0000026F  B000              mov al,0x0
00000271  EE                out dx,al
00000272  5A                pop dx
00000273  EBFE              jmp short 0x273
