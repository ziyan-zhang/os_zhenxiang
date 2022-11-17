[bits 32]
section .text
global switch_to
switch_to:
    ; 栈中此处是返回地址
    push esi;
    push edi;
    push ebx;
    push ebp;

    mov eax, [esp + 20]
    mov [eax], esp


    ;--------以上是备份当前线程环境; 以下是恢复下一个线程环境----------
    mov eax, [esp + 24]
    mov esp, [eax]

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret         ; 返回到上面switch_to下面的那句注释的返回地址; 未由中断进入的, 即第一次执行时会返回到kernel_thread
