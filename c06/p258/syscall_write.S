section .data
str_c_lib: db "c library says: hello world!", 0xa
str_c_lib_len equ $-str_c_lib

str_syscall: db "syscall says: hello world!", 0xa
str_syscall_len equ $-str_syscall
;str_syscall_len_len equ $-str_syscall_len

section .text
global _start
_start:
; 方式1：模拟语言中系统调用库函数write.
    push str_c_lib_len
    push str_c_lib
    push 1

    call simu_write
    add esp, 12                     ;回收栈空间，三个参数共12字节

; 方式2：跨过库函数，直接进行系统调用
    mov eax, 4
    mov ebx, 1
    mov ecx, str_syscall
    mov edx, str_syscall_len
    int 0x80

; 退出程序
    mov eax, 1
    int 0x80


simu_write:
    push ebp
    mov ebp,esp
    mov eax,4
    mov ebx, [ebp+8]
    mov ecx, [ebp+12]
    mov edx, [ebp+16]
    int 0x80
    pop ebp
    ret
