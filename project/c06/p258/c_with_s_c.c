extern void asm_print(char*, int);  // 只有参数类型，没有形参
void c_print(char* str) {
    int len=0;
    while(str[len++]);
    asm_print(str, len);
}