nasm -f elf -o c_with_s_s.o s_with_s_s.S
gcc -m32 -c -o c_with_s_c.o c_with_s_c.c
ld -melf_i386 -o c_with_s.bin c_with_s_s.o c_with_s_c.o
./c_with_s.bin
