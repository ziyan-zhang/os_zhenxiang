test2.o: test2.c
test1.o: test1.c
objfiles = test1.o test2.o
test.bin: $(objfiles)
	@echo gcc -o test.bin $(objfiles)
all: test.bin
	@echo "compile done"