gcc -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c -m32 -fno-stack-protector
nasm -f elf -o build/print.o lib/kernel/print.S
nasm -f elf -o build/kernel.o kernel/kernel.S
gcc -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c -m32 -fno-stack-protector
gcc -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c -m32 -fno-stack-protector
ld -Ttext 0xc0001500 -e main -o build/kernel.bin build/init.o build/main.o build/interrupt.o build/kernel.o build/print.o -melf_i386

strip --remove-section=.note.gnu.property build/kernel.bin

sudo dd if=build/kernel.bin of=/opt/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
sudo /opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk
