#include <unistd.h>
#include <stdio.h>
int main() {
    printf("I will fork in 1 second\n");
    sleep(1);
    int pid = fork();
    if (pid == -1) {
        return -1;
    }
    // if (pid) {
    //     printf("pid %d, I am father, my pid is %d\n", pid, getpid());
    //     sleep(5);
    //     return 0;
    // } else {
    //     printf("pid %d, I am child, my pid is %d\n", pid, getpid());
    //     sleep(5);
    //     return 0;
    // }
    printf("pid %d, I am? my pid is %d\n", pid, getpid());
    while(1);

}