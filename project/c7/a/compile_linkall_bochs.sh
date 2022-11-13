gcc -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c -m32
nasm -f elf -o build/print.o lib/kernel/print.S
nasm -f elf -o build/kernel.o kernel/kernel.S
gcc -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c -m32
gcc -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c -m32
ld -Ttext 0xc0001500 -e main -o build/kernel.bin build/init.o build/main.o build/interrupt.o build/kernel.o build/print.o -melf_i386
sudo dd if=build/kernel.bin of=/opt/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
sudo /opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk
