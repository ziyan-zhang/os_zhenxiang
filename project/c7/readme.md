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
好复杂的依赖关系,等学会了用一下第八章要学的makefile知识
