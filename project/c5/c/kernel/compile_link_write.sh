gcc -c -m32 -o main.o main.c
ld main.o -Ttext 0xc0001500 -e main -o kernel.bin -melf_i386
sudo dd if=kernel.bin of=/opt/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc

