00007C00  E9EC00            jmp 0x7cef
00007C03  4F                dec di
00007C04  7342              jnc 0x7c48
00007C06  6F                outsw
00007C07  6F                outsw
00007C08  7420              jz 0x7c2a
00007C0A  2000              and [bx+si],al
00007C0C  0201              add al,[bx+di]
00007C0E  0100              add [bx+si],ax
00007C10  02E0              add ah,al
00007C12  00400B            add [bx+si+0xb],al
00007C15  F00900            lock or [bx+si],ax
00007C18  1200              adc al,[bx+si]
00007C1A  0200              add al,[bx+si]
00007C1C  0000              add [bx+si],al
00007C1E  0000              add [bx+si],al
00007C20  0000              add [bx+si],al
00007C22  0000              add [bx+si],al
00007C24  0000              add [bx+si],al
00007C26  2900              sub [bx+si],ax
00007C28  0000              add [bx+si],al
00007C2A  00626F            add [bp+si+0x6f],ah
00007C2D  6F                outsw
00007C2E  7420              jz 0x7c50
00007C30  6C                insb
00007C31  6F                outsw
00007C32  61                popa
00007C33  64657246          gs jc 0x7c7d
00007C37  41                inc cx
00007C38  54                push sp
00007C39  3132              xor [bp+si],si
00007C3B  4C                dec sp
00007C3C  4F                dec di
00007C3D  41                inc cx
00007C3E  44                inc sp
00007C3F  45                inc bp
00007C40  52                push dx
00007C41  2020              and [bx+si],ah
00007C43  42                inc dx
00007C44  49                dec cx
00007C45  4E                dec si
00007C46  53                push bx
00007C47  7461              jz 0x7caa
00007C49  7274              jc 0x7cbf
00007C4B  20426F            and [bp+si+0x6f],al
00007C4E  6F                outsw
00007C4F  742D              jz 0x7c7e
00007C51  2D2D2D            sub ax,0x2d2d
00007C54  2D4E6F            sub ax,0x6f4e
00007C57  7420              jz 0x7c79
00007C59  46                inc si
00007C5A  6F                outsw
00007C5B  756E              jnz 0x7ccb
00007C5D  64206C6F          and [fs:si+0x6f],ch
00007C61  61                popa
00007C62  6465722E          gs jc 0x7c94
00007C66  62696E            bound bp,[bx+di+0x6e]
00007C69  005589            add [di-0x77],dl
00007C6C  E566              in ax,0x66
00007C6E  83EC02            sub sp,byte +0x2
00007C71  884EFE            mov [bp-0x2],cl
00007C74  53                push bx
00007C75  31D2              xor dx,dx
00007C77  8A1E187C          mov bl,[0x7c18]
00007C7B  F6F3              div bl
00007C7D  FEC4              inc ah
00007C7F  88E1              mov cl,ah
00007C81  88C6              mov dh,al
00007C83  30E4              xor ah,ah
00007C85  8A1E1A7C          mov bl,[0x7c1a]
00007C89  F6F3              div bl
00007C8B  88C5              mov ch,al
00007C8D  88E6              mov dh,ah
00007C8F  5B                pop bx
00007C90  8A16247C          mov dl,[0x7c24]
00007C94  B402              mov ah,0x2
00007C96  8A46FE            mov al,[bp-0x2]
00007C99  CD13              int 0x13
00007C9B  72F7              jc 0x7c94
00007C9D  6683C402          add esp,byte +0x2
00007CA1  5D                pop bp
00007CA2  C3                ret
00007CA3  06                push es
00007CA4  53                push bx
00007CA5  50                push ax
00007CA6  B80000            mov ax,0x0
00007CA9  8EC0              mov es,ax
00007CAB  58                pop ax
00007CAC  C606697C00        mov byte [0x7c69],0x0
00007CB1  BB0300            mov bx,0x3
00007CB4  F7E3              mul bx
00007CB6  BB0200            mov bx,0x2
00007CB9  F7F3              div bx
00007CBB  83FA00            cmp dx,byte +0x0
00007CBE  7405              jz 0x7cc5
00007CC0  C606697C01        mov byte [0x7c69],0x1
00007CC5  31D2              xor dx,dx
00007CC7  8B1E0B7C          mov bx,[0x7c0b]
00007CCB  F7F3              div bx
00007CCD  52                push dx
00007CCE  BB0080            mov bx,0x8000
00007CD1  83C001            add ax,byte +0x1
00007CD4  B102              mov cl,0x2
00007CD6  E891FF            call 0x7c6a
00007CD9  5A                pop dx
00007CDA  01D3              add bx,dx
00007CDC  268B07            mov ax,[es:bx]
00007CDF  803E697C01        cmp byte [0x7c69],0x1
00007CE4  7503              jnz 0x7ce9
00007CE6  C1E804            shr ax,byte 0x4
00007CE9  25FF0F            and ax,0xfff
00007CEC  5B                pop bx
00007CED  07                pop es
00007CEE  C3                ret
00007CEF  8CC8              mov ax,cs
00007CF1  8ED8              mov ds,ax
00007CF3  8EC0              mov es,ax
00007CF5  8ED0              mov ss,ax
00007CF7  BC007C            mov sp,0x7c00
00007CFA  8816247C          mov [0x7c24],dl
00007CFE  B80006            mov ax,0x600
00007D01  BB0007            mov bx,0x700
00007D04  B90000            mov cx,0x0
00007D07  BA4F18            mov dx,0x184f
00007D0A  CD10              int 0x10
00007D0C  B80002            mov ax,0x200
00007D0F  BB0000            mov bx,0x0
00007D12  BA0000            mov dx,0x0
00007D15  CD10              int 0x10
00007D17  B80113            mov ax,0x1301
00007D1A  BB0F00            mov bx,0xf
00007D1D  BA0000            mov dx,0x0
00007D20  B90F00            mov cx,0xf
00007D23  50                push ax
00007D24  8CD8              mov ax,ds
00007D26  8EC0              mov es,ax
00007D28  58                pop ax
00007D29  BD467C            mov bp,0x7c46
00007D2C  CD10              int 0x10
00007D2E  B81300            mov ax,0x13
00007D31  B90E00            mov cx,0xe
00007D34  BB0080            mov bx,0x8000
00007D37  E830FF            call 0x7c6a
00007D3A  8B16117C          mov dx,[0x7c11]
00007D3E  83FA00            cmp dx,byte +0x0
00007D41  7421              jz 0x7d64
00007D43  4A                dec dx
00007D44  BE3B7C            mov si,0x7c3b
00007D47  B90B00            mov cx,0xb
00007D4A  FC                cld
00007D4B  83F900            cmp cx,byte +0x0
00007D4E  7414              jz 0x7d64
00007D50  49                dec cx
00007D51  89DF              mov di,bx
00007D53  43                inc bx
00007D54  AC                lodsb
00007D55  263A05            cmp al,[es:di]
00007D58  74F1              jz 0x7d4b
00007D5A  EB00              jmp short 0x7d5c
00007D5C  83E3E0            and bx,byte -0x20
00007D5F  83C320            add bx,byte +0x20
00007D62  EBDA              jmp short 0x7d3e
00007D64  83FA00            cmp dx,byte +0x0
00007D67  7402              jz 0x7d6b
00007D69  EB19              jmp short 0x7d84
00007D6B  B80113            mov ax,0x1301
00007D6E  BB8C00            mov bx,0x8c
00007D71  BA0001            mov dx,0x100
00007D74  B91400            mov cx,0x14
00007D77  50                push ax
00007D78  8CD8              mov ax,ds
00007D7A  8EC0              mov es,ax
00007D7C  58                pop ax
00007D7D  BD557C            mov bp,0x7c55
00007D80  CD10              int 0x10
00007D82  EBFE              jmp short 0x7d82
00007D84  83E7E0            and di,byte -0x20
00007D87  83C71A            add di,byte +0x1a
00007D8A  268B05            mov ax,[es:di]
00007D8D  BA0010            mov dx,0x1000
00007D90  8EC2              mov es,dx
00007D92  BB0000            mov bx,0x0
00007D95  50                push ax
00007D96  53                push bx
00007D97  B40E              mov ah,0xe
00007D99  B02E              mov al,0x2e
00007D9B  B30F              mov bl,0xf
00007D9D  CD10              int 0x10
00007D9F  5B                pop bx
00007DA0  58                pop ax
00007DA1  89C1              mov cx,ax
00007DA3  83C10E            add cx,byte +0xe
00007DA6  83C111            add cx,byte +0x11
00007DA9  50                push ax
00007DAA  89C8              mov ax,cx
00007DAC  B101              mov cl,0x1
00007DAE  E8B9FE            call 0x7c6a
00007DB1  031E0B7C          add bx,[0x7c0b]
00007DB5  58                pop ax
00007DB6  E8EAFE            call 0x7ca3
00007DB9  3DFF0F            cmp ax,0xfff
00007DBC  75D7              jnz 0x7d95
00007DBE  EB00              jmp short 0x7dc0
00007DC0  EA00000010        jmp 0x1000:0x0
00007DC5  0000              add [bx+si],al
00007DC7  0000              add [bx+si],al
00007DC9  0000              add [bx+si],al
00007DCB  0000              add [bx+si],al
00007DCD  0000              add [bx+si],al
00007DCF  0000              add [bx+si],al
00007DD1  0000              add [bx+si],al
00007DD3  0000              add [bx+si],al
00007DD5  0000              add [bx+si],al
00007DD7  0000              add [bx+si],al
00007DD9  0000              add [bx+si],al
00007DDB  0000              add [bx+si],al
00007DDD  0000              add [bx+si],al
00007DDF  0000              add [bx+si],al
00007DE1  0000              add [bx+si],al
00007DE3  0000              add [bx+si],al
00007DE5  0000              add [bx+si],al
00007DE7  0000              add [bx+si],al
00007DE9  0000              add [bx+si],al
00007DEB  0000              add [bx+si],al
00007DED  0000              add [bx+si],al
00007DEF  0000              add [bx+si],al
00007DF1  0000              add [bx+si],al
00007DF3  0000              add [bx+si],al
00007DF5  0000              add [bx+si],al
00007DF7  0000              add [bx+si],al
00007DF9  0000              add [bx+si],al
00007DFB  0000              add [bx+si],al
00007DFD  0055AA            add [di-0x56],dl
00007E00  F0                lock
00007E01  FF                db 0xff
00007E02  FF                db 0xff
