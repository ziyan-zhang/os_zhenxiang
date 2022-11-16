gcc -c -o reg6.o reg6.c -m32
ld -Ttext 0xc0001500 -e main -o reg6.bin reg6.o -melf_i386 -shared
# -lc替换-shared也能链接通过,但是不能运行.bin
