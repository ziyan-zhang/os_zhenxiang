   %include "boot.inc"
   section loader vstart=LOADER_BASE_ADDR
   LOADER_STACK_TOP equ LOADER_BASE_ADDR
;   jmp loader_start					; 此处的物理地址是:
   
;构建gdt及其内部的描述符
   GDT_BASE:   dd    0x00000000 
	       dd    0x00000000

   CODE_DESC:  dd    0x0000FFFF 
	       dd    DESC_CODE_HIGH4

   DATA_STACK_DESC:  dd    0x0000FFFF
		     dd    DESC_DATA_HIGH4

   VIDEO_DESC: dd    0x80000007	       ;limit=(0xbffff-0xb8000)/4k=0x7
	       dd    DESC_VIDEO_HIGH4  ; 此时dpl已改为0

   GDT_SIZE   equ   $ - GDT_BASE
   GDT_LIMIT   equ   GDT_SIZE -	1 
   times 60 dq 0					 ; 此处预留60个描述符的slot
   SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0         ; 相当于(CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0
   SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0	 ; 同上
   SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0	 ; 同上 

   total_mem_bytes dd 0
   ;以下是定义gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址

   gdt_ptr  dw  GDT_LIMIT 
	    dd  GDT_BASE

   ards_buf times 244 db 0
   ards_nr dw 0
   
   loader_start:
	xor ebx, ebx
	mov edx, 0x534d4150
	mov di, ards_buf
.e820_mem_get_loop:
	mov eax, 0x0000e820
	mov ecx, 20
	int 0x15
	jc .e820_failed_so_try_e801
	add di, cx
	inc word [ards_nr]
	cmp ebx, 0
	jnz .e820_mem_get_loop

	mov cx, [ards_nr]
	mov ebx, ards_buf
	xor edx, edx
.find_max_mem_area:
	mov eax, [ebx]
	add eax, [ebx+8]
	add ebx, 20
	cmp edx, eax
	jge .next_ards
	mov edx, eax
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok

; -----  int 15h ax = e801h 获取内存大小，最大支持4G -----
.e820_failed_so_try_e801:
	mov ax, 0xe801
	int 0x15
	jc .e801_failed_so_try88

	mov cx, 0x400
	mul cx
	shl edx, 16
	and eax, 0x0000FFFF
	or edx, eax
	and edx, 0x100000
	mov esi, edx

	xor eax, eax
	mov ax, bx
	mov ecx, 0x10000
	mul ecx
	
	add esi, eax
	mov edx, esi
	jmp .mem_get_ok

; ----- int 15h ah = 0x88 获取内存大小，只能获取64MB之内
.e801_failed_so_try88:
	mov ah, 0x88
	int 0x15
	jc .error_hlt
	and eax, 0x0000FFFF

	mov cx, 0x400
	mul cx
	shl edx, 16
	or edx, eax
	add edx, 0x100000

.mem_get_ok:
	mov [total_mem_bytes], edx

; ----- 准备进入保护模式 -----
; ---1 打开A20 ---
	in al, 0x92
	or al, 0000_0010B
	out 0x92, al
; ---2 加载GDT---
	lgdt [gdt_ptr]
; ---3 将cr0的pe位置1
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax
	
	jmp dword SELECTOR_CODE:p_mode_start

.error_hlt:
	hlt

[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax

	mov byte [gs:160], 'P'

	call setup_page
	sgdt [gdt_ptr]
	mov ebx, [gdt_ptr+2]
	or dword [ebx+0x18+4], 0xc0000000
	add dword [gdt_ptr+2], 0xc0000000

	add esp, 0xc0000000
	
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	lgdt [gdt_ptr]

	mov byte [gs:160], 'V'

	jmp $ 

setup_page:
	mov ecx, 4096
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

.create_pde:
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000
	mov ebx, eax
	
	or eax, PG_US_U | PG_RW_W | PG_P
	mov [PAGE_DIR_TABLE_POS + 0x0], eax
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax

	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax

	mov ecx, 256
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
	mov [ebx+esi*4], edx
	add edx, 4096
	inc esi
	loop .create_pte

	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000
	or eax, PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254
	mov esi, 769
.create_kernel_pde:
	mov [ebx+esi*4], eax
	inc esi
	add eax, 0x1000
	loop .create_kernel_pde
	ret
