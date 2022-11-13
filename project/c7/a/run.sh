#!/bin/bash

# 0. 目录和路径
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

# 1. 用文件创建虚拟磁盘, 写mbr和loader
echo -e "\033[32m用文件创建虚拟磁盘, 写mbr和loader \033[0m"

/opt/bochs/bin/bximage -hd -mode="flat" -size=60 -q hd60M.img 
nasm -I ./boot/include/ -o ./build/mbr.bin ./boot/mbr.S && dd if=./build/mbr.bin of=./hd60M.img bs=512 count=1  conv=notrunc 
nasm -I ./boot/include/ -o ./build/loader.bin ./boot/loader.S && dd if=./build/loader.bin of=./hd60M.img bs=512 count=4 seek=2 conv=notrunc 

# 2. 编译, 为kernel.bin作准备
echo -e "\033[32m编译, 各种 .o 文件 \033[0m"

gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c
nasm -f elf -o build/print.o lib/kernel/print.S
nasm -f elf -o build/kernel.o kernel/kernel.S

gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/interrupt.o kernel/interrupt.c
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c


# 3. 链接
echo -e "\033[32m链接, 注意顺序, 尤其 main.o 在最前 \033[0m"

# 下面的这个main.o在第一个, 正常触发时钟中断
ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/kernel.o build/print.o

# while 下面的这个main.o在第二个, fail
# ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/init.o build/main.o build/interrupt.o build/kernel.o build/print.o
# 类似地, 将 print.o 提前, 直接只能打最开始的 mbr 三个字符了.
# ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/print.o build/main.o build/init.o build/interrupt.o build/kernel.o



# 4. 写盘和拷贝, 运行
echo -e "\033[32m写盘,拷贝和运行,后两个需要根权限 \033[0m"

dd if=build/kernel.bin of=./hd60M.img bs=512 count=200 seek=9 conv=notrunc
cp ./hd60M.img /opt/bochs/hd60M.img
/opt/bochs/bin/bochs -f /opt/bochs/bochsrc.disk
