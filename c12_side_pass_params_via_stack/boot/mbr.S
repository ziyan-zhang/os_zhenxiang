;主引导程序
;-------------------------------------------------
%include "boot.inc"
SECTION MBR vstart=0x7c00
	mov ax,cs
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov fs,ax
	mov sp,0x7c00
	mov ax,0xb800
	mov gs,ax

;清屏
;利用0x06号功能,上卷全部行,则可清屏
;-------------------------------
;INT 0x10	功能号:0x06		功能描述:上卷窗口
;-------------------------------

	mov ax,0600h
	mov bx,0700h
	mov cx,0
	mov dx,184fh

	int 10h

	; 输出字符串:MBR
	mov byte [gs:0x00],'1'
	mov byte [gs:0x01],0xA4

	mov byte [gs:0x02],' '
	mov byte [gs:0x03],0xA4

	mov byte [gs:0x04],'M'
	mov byte [gs:0x05],0xA4		;A表示绿色背景闪烁,4表示前景色为红色

	mov byte [gs:0x06],'B'
	mov byte [gs:0x07],0xA4

	mov byte [gs:0x08],'R'
	mov byte [gs:0x09],0xA4

	mov eax,LOADER_START_SECTOR		;起始扇区lba地址
	mov bx,LOADER_BASE_ADDR			;写入的地址
	mov cx,4						;待读入的扇区数
	call rd_disk_m_16				;读取程序的起始部分(一个扇区)

	jmp LOADER_BASE_ADDR + 0x300 ; 这是较c04改动的地方

rd_disk_m_16:

	mov esi,eax
	mov di,cx
	mov dx,0x1f2
	mov al,cl
	out dx,al
	mov eax,esi

	mov dx,0x1f3
	out dx,al
	mov cl,8
	shr eax,cl
	mov dx,0x1f4
	out dx,al
	shr eax,cl
	mov dx,0x1f5
	out dx,al
	shr eax,cl
	and al,0x0f
	or al,0xe0
	mov dx,0x1f6
	out dx,al
	mov dx,0x1f7
	mov al,0x20
	out dx,al

  .not_ready:
	nop
	in al,dx
	and al,0x88
	cmp al,0x08
	jnz .not_ready

	mov ax,di
	mov dx,256
	mul dx
	mov cx,ax
	mov dx,0x1f0
  .go_on_read:
	in ax,dx
	mov [bx],ax
	add bx,2
	loop .go_on_read
	ret
times 510-($-$$) db 0
db 0x55,0xaa

	

