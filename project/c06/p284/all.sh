gcc -c -o base_asm.o base_asm.c -m32
ld -Ttext 0xc0001500 -e main -o base_asm.bin \
 base_asm.o -melf_i386
./base_asm.bin
