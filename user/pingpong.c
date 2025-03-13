#include "kernel/types.h"
#include "user/user.h"


#define WRITE 1
#define READ 0


#define stdin 0
#define stdout 1
#define stderr 2

int main(int argc, char** argv)//char *的一个指针！！！
{
    int fdp[2], fdc[2], pid;


    pipe(fdp);
    pipe(fdc);

    if((pid = fork()) < 0)
    {
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if(pid == 0)//child  read msg from parent, and send pong to its father
    {
        char buf;
        close(fdc[READ]); // notice : parent and child process both have the pipe fd 
        close(fdp[WRITE]);// so closing the write side of parent pipe doesn't have impact on parent side

        read(fdp[READ], &buf, 1);
        close(fdp[READ]);

        printf("%d: received ping\n", getpid());//注意这里的输出格式“pid: ”必须严格这样否则无法通过!!! 
        write(fdc[WRITE], &buf, 1);
        close(fdc[WRITE]);

        exit(0);
    }
    else
    {
        char buf;
        close(fdp[READ]);
        close(fdc[WRITE]);

        write(fdp[WRITE], "h", 1);
        close(fdp[WRITE]);

        read(fdc[READ], &buf, 1);
        close(fdc[READ]);

        printf("%d: received pong\n", getpid());

        wait(0);

    }
    exit(0);
}