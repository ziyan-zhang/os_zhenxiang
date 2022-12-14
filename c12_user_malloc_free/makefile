BUILD_DIR = ./build
DISK_IMG = hd3M.img
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/ -I thread/ -I userprog/
ASFLAGS = -f elf 
ASBINLIB = -I boot/include/
CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes \
         -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -melf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
# OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
#       $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
#       $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o \
# 	  $(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/switch.o \
# 	  $(BUILD_DIR)/list.o

OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
      $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
      $(BUILD_DIR)/debug.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/bitmap.o \
	$(BUILD_DIR)/string.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o \
	$(BUILD_DIR)/switch.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o \
	$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o \
	$(BUILD_DIR)/process.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscall-init.o \
	$(BUILD_DIR)/stdio.o

##############     MBR和bootloader代码编译     ############### 
$(BUILD_DIR)/mbr.bin: boot/mbr.S 
	$(AS) $(ASBINLIB) $< -o $@
$(BUILD_DIR)/loader.bin: boot/loader.S 
	$(AS) $(ASBINLIB) $< -o $@

##############     c代码编译     ###############
$(BUILD_DIR)/main.o: kernel/main.c \
		lib/kernel/print.h kernel/init.h kernel/debug.h \
		kernel/memory.h thread/thread.h kernel/interrupt.h \
		device/console.h device/ioqueue.h device/keyboard.h \
		userprog/process.h userprog/syscall-init.h \
		lib/user/syscall.h lib/stdio.h
	$(CC) $(CFLAGS) $< -o $@


$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h \
		lib/kernel/print.h kernel/interrupt.h \
		device/timer.h kernel/memory.h thread/thread.h \
		device/console.h device/keyboard.h userprog/tss.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h \
		lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h \
		lib/kernel/io.h lib/kernel/print.h lib/stdint.h \
		thread/thread.h kernel/interrupt.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h \
		lib/kernel/print.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string.c lib/string.h \
		kernel/global.h kernel/debug.h lib/stdint.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h \
		lib/stdint.h lib/string.h lib/kernel/print.h \
		kernel/interrupt.h kernel/debug.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h \
		lib/kernel/bitmap.h lib/stdint.h kernel/global.h \
		kernel/debug.h lib/kernel/print.h lib/string.h \
		thread/sync.h lib/kernel/list.h kernel/interrupt.h \
		kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h \
		lib/stdint.h lib/string.h kernel/global.h \
		kernel/memory.h lib/kernel/list.h \
		kernel/interrupt.h kernel/debug.h lib/kernel/print.h \
		kernel/memory.h userprog/process.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o: lib/kernel/list.c lib/kernel/list.h \
		kernel/interrupt.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sync.o: thread/sync.c thread/sync.h \
		kernel/interrupt.h lib/kernel/list.h \
		lib/stdint.h thread/thread.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/console.o: device/console.c device/console.h \
		lib/kernel/print.h lib/stdint.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: device/keyboard.c device/keyboard.h \
		lib/kernel/print.h kernel/interrupt.h lib/kernel/io.h \
		kernel/global.h device/ioqueue.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/ioqueue.o: device/ioqueue.c device/ioqueue.h \
		kernel/interrupt.h kernel/global.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss.o: userprog/tss.c userprog/tss.h \
		lib/stdint.h kernel/memory.h kernel/global.h \
		lib/kernel/print.h thread/thread.h lib/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o: userprog/process.c userprog/process.h \
		thread/thread.h kernel/global.h kernel/memory.h \
		kernel/interrupt.h lib/string.h lib/stdint.h \
		kernel/debug.h userprog/tss.h device/console.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: lib/user/syscall.c lib/user/syscall.h \
		lib/stdint.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c userprog/syscall-init.h \
		lib/user/syscall.h thread/thread.h lib/stdint.h \
		lib/kernel/print.h device/console.h lib/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio.o: lib/stdio.c lib/stdio.h \
		lib/stdint.h kernel/global.h \
		lib/kernel/print.h lib/string.h lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@

##############    汇编代码编译    ###############
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/switch.o: thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
		$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean all bochs test1

mk_dir:
	if [ ! -d $(BUILD_DIR) ];then mkdir $(BUILD_DIR);fi  # 注意这个mkdir是bash的, 不能改成mk_dir

mk_img:
	if [ ! -e $(DISK_IMG) ];then /opt/bochs/bin/bximage -hd -mode="flat" -size=60 -q $(DISK_IMG);fi

hd:
	dd if=$(BUILD_DIR)/mbr.bin of=hd3M.img bs=512 count=1  conv=notrunc
	dd if=$(BUILD_DIR)/loader.bin of=hd3M.img bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin \
		of=hd3M.img bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin

cpcp:
	cp hd3M.img /opt/bochs/hd60M.img 

bochs:
	sudo /opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk

test1: $(BUILD_DIR)/thread.o

all: mk_dir clean mk_img build hd cpcp bochs
