a/ 中是正确的代码 \n
a_bk是博客 https://www.cnblogs.com/Lizhixing/p/16437788.html 中的代码

昨晚通宵抄和对比各种网上的真象还原第七章中断代码,都出现了0xE中断,而不是时钟中断


# 编译
创建hd60M.img, 写mbr.bin, 写loader.bin, 然后编译
见run.sh代码


# 链接尤其要注意(!!)
链接的时候, 注意顺序, 尤其 main.o 在最前, 可见其用了 print 和 init 的 .o 文件. 

昨天一夜没找到, 没有这个意识, 乱序抄的, 所以错了.

```cpp
#include "print.h"
#include "init.h"
void main(void) {
    put_str("I am kernel\n");
    init_all();
    asm volatile("sti");
    while(1);
}
```

下面的这个main.o在第一个, 正常触发时钟中断
```bash
ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/kernel.o build/print.o
```

while 下面的这个main.o在第二个, fail
```bash
ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/init.o build/main.o build/interrupt.o build/kernel.o build/print.o
```

类似地, 将 print.o 提前, 直接只能打最开始的 mbr 三个字符了.
```bash
ld -melf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/print.o build/main.o build/init.o build/interrupt.o build/kernel.o
```

markdown插入图片好像很麻烦,但确实是这样的.

# 链接顺序的规范和原理
注意, a.c include 了 b.h 的话, 链接时a的目标文件要放在b的目标文件前面, 也即要写成 ld a.o b.o, 而 ld b.o a.o 出来的文件有问题, 虽然ld本身并不出错. 

原理参考<操作系统真象还原274页>. 

在链接顺序上, 属于"调用在前, 实现在后"的顺序. 链接器最先处理的目标文件是参数从左边数第一个(main.o), 对于里面找不到的符号(put_char), 链接器会将它记录下来, 以备在后面的目标文件中(print.o)查找. 

如果将其顺序颠倒, 势必导致在后面的目标文件中才出现找不到符号的情况, 而该符号的实现所在的目标文件早已经过去了(后面的 .o 看不见前面的 .o 中的符号地址.), 这可能使链接器内部采用另外的处理流程, 导致出来的执行顺序不太一样. 
