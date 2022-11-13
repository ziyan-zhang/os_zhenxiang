gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c
nasm -f elf -o build/print.o lib/kernel/print.S
nasm -f elf -o build/kernel.o kernel/kernel.S

gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c

ld -melf_i386 -Ttext 0xc0001500 -e main -o kernel.bin build/main.o build/print.o build/init.o build/interrupt.o build/kernel.o
dd if=./kernel.bin of=./hd60M.img bs=512 count=200 seek=9 conv=notrunc 


cp ./hd60M.img /opt/bochs/hd60M.img
/opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk
