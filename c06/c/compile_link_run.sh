nasm -f elf -o lib/kernel/print.o lib/kernel/print.S
gcc -I lib/kernel/ -c -o kernel/main.o kernel/main.c -m32
ld -Ttext c0001500 -e main -o kernel.bin kernel/main.o lib/kernel/print.o -melf_i386
sudo dd if=kernel.bin of=/opt/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
sudo /opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk
