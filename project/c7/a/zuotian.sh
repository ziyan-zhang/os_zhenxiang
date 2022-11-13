# 编译
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c
nasm -f elf -o build/print.o lib/kernel/print.S
nasm -f elf -o build/kernel.o kernel/kernel.S

gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c


# 链接
# 下面的这个main.o在第二个, fail
ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/init.o build/main.o build/interrupt.o build/kernel.o build/print.o

# while下面的这个main.o在第一个, works
# ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/kernel.o build/print.o


# 写盘和拷贝
dd if=build/kernel.bin of=/opt/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
/opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk
