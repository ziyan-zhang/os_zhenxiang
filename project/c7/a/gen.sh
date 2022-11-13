#!/bin/bash
if [ ! -d "./build" ]; then
    mkdir build
fi
if [ -e "hd60M.img" ]; then
    rm -rf hd60M.img  
fi
if [ ! -d "./build/boot" ];then 
    mkdir build/boot
fi 
if [ ! -d "./build/kernel" ];then 
    mkdir build/kernel 
fi 

echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成hd60M.img \033[0m"
/opt/bochs/bin/bximage -hd -mode="flat" -size=60 -q hd60M.img 
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成build/boot/mbr.bin \033[0m"
nasm -I ./boot/include/ -o ./build/boot/mbr.bin ./boot/mbr.S && dd if=./build/boot/mbr.bin of=./hd60M.img bs=512 count=1  conv=notrunc 
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成build/boot/loader.bin \033[0m"
nasm -I ./boot/include/ -o ./build/boot/loader.bin ./boot/loader.S && dd if=./build/boot/loader.bin of=./hd60M.img bs=512 count=4 seek=2 conv=notrunc 
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成build/kernel/print.o \033[0m"
nasm -f elf -o build/kernel/print.o lib/kernel/print.S
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成kernel.o \033[0m"
nasm -f elf -o build/kernel/kernel.o kernel/kernel.S
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成kernel.i \033[0m"
nasm -E -o kernel/kernel.i kernel/kernel.S
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成build/kernel/main.o \033[0m"
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/kernel/main.o kernel/main.c
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成build/interrupt.o \033[0m"
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c
echo -e "\033[32m==================================================================================================== \033[0m \033[0m"
echo -e "\033[32m生成build/init.o"
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c
echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m生成build/kernel/kernel.bin mkimage \033[0m"

ld -melf_i386 -Ttext 0xc0001500 -e main -o ./build/kernel/kernel.bin \
    build/kernel/main.o build/kernel/print.o build/init.o build/interrupt.o build/kernel/kernel.o \
    && dd if=./build/kernel/kernel.bin of=./hd60M.img bs=512 count=200 seek=9 conv=notrunc 

echo -e "\033[32m==================================================================================================== \033[0m"
tree build/

echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m将本目录下生成的hd60M拷贝到/opt/bochs/目录下,需要根权限 \033[0m"
cp ./hd60M.img /opt/bochs/hd60M.img

echo -e "\033[32m==================================================================================================== \033[0m"
echo -e "\033[32m运行bochs上运行,hd60M.img上的操作系统,需要根权限 \033[0m"
/opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk