TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

section .data
put_int_buffer dq 0         ; 定义8字节缓冲区用于数字到字符的转换

[bits 32]
section .text
global put_str
put_str:
    push ebx
    push ecx
    xor ecx, ecx
    mov ebx, [esp + 12]
.goon:
    mov cl, [ebx]
    cmp cl, 0
    jz .str_over
    push ecx
    call put_char
    add esp, 4
    inc ebx
    jmp .goon
.str_over:
    pop ecx
    pop ebx
    ret

;----------------put char-------------------
global put_char
put_char:
    pushad
    mov ax, SELECTOR_VIDEO
    mov gs, ax


    ;;;;;;;;获取光标当前位置;;;;;;;;;;;;
    ;先获得高8位, 第一步设置地址寄存器
    mov dx, 0x03d4  ; 0x03d4是CRT Controller寄存器组的Address Register的端口地址
    mov al, 0x0e    ; 0x0e  是CTR Controller数据寄存器组中光标位置高8位寄存器的索引
    out dx, al      ; 把光标高8位的寄存器索引 写入 CRT Controller寄存器组的Address Register; 此时data register就能出高八位的地址(地址就是寄存器中存的内容)了
    mov dx, 0x03d5  ; 第二步, 读取数据寄存器CRT Controller寄存器组的Data Register的端口地址为0x3D5
    in al, dx       ; 通过Data Register写数据到al寄存器
    mov ah, al      ; 把高八位存到ah寄存器

    ; 再获取低8位,于此类似.
    mov dx, 0x03d4  ; 第一步, 设置地址寄存器
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5  ; 第二步, 读取数据寄存器
    in al, dx

    ; 将光标存入bx, 获取待打印的字符
    mov bx, ax
    mov ecx, [esp + 36]
    ; 处理特殊情况
    cmp cl, 0xd
    jz .is_carriage_return
    cmp cl, 0xa
    jz .is_line_feed

    cmp cl, 0x8
    jz .is_backspace
    jmp .put_other
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.is_backspace:




    dec bx
    shl bx,1

    mov byte [gs:bx], 0x20
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    jmp .set_cursor

.put_other:
    shl bx, 1
    mov [gs:bx], cl
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    inc bx
    cmp bx, 2000
    jl .set_cursor

.is_line_feed:
.is_carriage_return:
; 如果是回车符,只要把光标移到行首就可以了
    xor dx, dx
    mov ax, bx
    mov si, 80
    div si          ; dx中为余数
    sub bx, dx


.is_carriage_return_end:
    add bx, 80
    cmp bx, 2000
.is_line_feed_end:
    jl .set_cursor

; 屏幕行范围是0~24, 滚屏的原理是将屏幕的第1~24行搬运到第0~23行; 再将24行用空格填充
.roll_screen:
    cld
    mov ecx, 960
    ; mov esi, 0xc00b80a0
    ; mov edi, 0xc00b8000
    mov esi, 0xb80a0
    mov edi, 0xb8000
    rep movsd

;;;;;将最后一行填充为空白
    mov ebx, 3840
    mov ecx, 80
.cls:
    mov word [gs:ebx], 0x0720
    add ebx, 2
    loop .cls
    mov bx, 1920

global .set_cursor
.set_cursor:
; 将光标设置为bx值
;;;;;;;;1 先设置高8位
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al

;;;;;;;;2 再设置低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al
.put_char_done:
    popad
    ret

;-----------------    put_int     --------------
global put_int
put_int:
    pushad
    mov ebp, esp
    mov eax, [ebp+4*9]
    mov edx, eax
    mov edi, 7
    mov ecx, 8
    mov ebx, put_int_buffer

.16based_4bits:
    and edx, 0x0000000F
    cmp edx, 9
    jg .is_A2F
    add edx, '0'
    jmp .store
.is_A2F:
    sub edx, 10
    add edx, 'A'

.store:
    mov [ebx+edi], dl
    dec edi
    shr eax, 4
    mov edx, eax
    loop .16based_4bits

.ready_to_print:
    inc edi
.skip_prefix_0:
    cmp edi, 8
    je .full0

.go_on_skip:
    mov cl, [put_int_buffer+edi]
    inc edi
    cmp cl, '0'
    je .skip_prefix_0
    dec edi
    jmp .put_each_num

.full0:
    mov cl, '0'
.put_each_num:
    push ecx
    call put_char
    add esp, 4
    inc edi
    mov cl, [put_int_buffer+edi]
    cmp edi, 8
    jl .put_each_num
    popad
    ret
