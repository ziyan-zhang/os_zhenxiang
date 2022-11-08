nasm -I ./boot/include/ -o ./boot/loader.bin ./boot/loader.S
sudo dd if=./boot/loader.bin of=/opt/bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc # 注意这里loader.bin的大小为1752字节，至少要写4扇区，否则后面的代码不会运行

gcc -c -m32 -o ./kernel/main.o ./kernel/main.c
ld ./kernel/main.o -Ttext 0xc0001500 -e main -o ./kernel/kernel.bin -melf_i386
sudo dd if=./kernel/kernel.bin of=/opt/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc

sudo /opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk
