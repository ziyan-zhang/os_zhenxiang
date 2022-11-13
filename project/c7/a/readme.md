a/ 中是正确的代码 \n
a_bk是博客 https://www.cnblogs.com/Lizhixing/p/16437788.html 中的代码

昨晚通宵抄和对比各种网上的真象还原第七章中断代码,都出现了0xE中断,而不是时钟中断,今天试了下从头开始写盘mbr.bin和loader.bin,正常了,说明之前写错了(或者某次给改了)

# 第零块,创建hd60M.img
使用bochs带的bximage功能
sudo /opt/bochs/bin/bximage -hd -mode="flat" -size=60 -q /opt/bochs/hd60M.img

# 第一块,写mbr.bin
最近一次写mbr.S是在c5/b/,复制过来到c7/a/boot,参考作者代码加点注释顺便复习一下. 这里顺便要拷贝编译mbr.S时要用到的boot.inc

nasm -I ./boot/include/ -o ./build/mbr.bin ./boot/mbr.S
注意include后面的/不能忘,否则打不开boot.inc

sudo dd if=./build/mbr.bin of=/opt/bochs/hd60M.img bs=512 count=1 conv=notrunc

这里hd60M.img即使不存在,写完就存在了,就是不知道会不会有后续问题.

# 第二块,写loader.bin
loader.bin最近出现的位置是c5/c/boot
先在boot.inc里面更新一些配置
nasm -I ./boot/include/ -o build/loader.bin boot/loader.S 
sudo dd if=./build/loader.bin of=/opt/bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc

# 第三块,写kernel.bin
注意, a.c include 了 b.h 的话, 链接时a的目标文件要放在b的目标文件前面, 也即要写成 ld a.o b.o, 而 ld b.o a.o 出来的文件有问题, 虽然ld本身并不出错. 原理应该是符号地址暂时找不到的画会暂时记下这个空, 向后找. 而后面的 .o 看不见前面的 .o 中的符号地址.
编译链接写在了脚本 zuotian.sh 中.

摘出来上面那段话相关的:

下面的这个main.o在第二个, fail
ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/init.o build/main.o build/interrupt.o build/kernel.o build/print.o

while下面的这个main.o在第一个, works
ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/kernel.o build/print.o

markdown插入图片好像很麻烦,但确实是这样的.