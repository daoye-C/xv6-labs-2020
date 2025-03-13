#include "kernel/types.h"
#include "user/user.h"

#define numprime 35
#define READ 0
#define WRITE 1
#define stdin 0
#define stdout 1
#define stderr 2


void 
sieve(int * left)
{
    close(left[WRITE]); //关闭左端文件描述符写端

    int prime, tmp, pid, right[2];
    
    if(read(left[READ], &prime, sizeof (int)) == 0)//从管道中读到0字节，则意味着已经完全输出了，该结束了
    {
        close(left[READ]);
        exit(0);
    }
    
    printf("prime %d\n", prime);

    pipe(right);

    if((pid = fork()) > 0)
    {
        close(right[READ]);
        
        while(read(left[READ], &tmp, sizeof tmp))
            if(tmp % prime != 0)
                write(right[WRITE], &tmp, sizeof tmp);
        

        close(right[WRITE]);
        wait(0);
        exit(0);
    }
    else
    {
        sieve(right);
        exit(0);
    }
}

int
main(int argc, char** argv)
{
    int input_pipe[2];

    pipe(input_pipe);

    int pid;

    if((pid = fork()) > 0)
    {
        close(input_pipe[READ]);

        
        for(int i = 2; i <= numprime ; i ++ )
            write(input_pipe[WRITE], &i , sizeof i);
        
        close(input_pipe[WRITE]);
        wait(0);
    }
    else
    {
        sieve(input_pipe);
        exit(0);
    }
    exit(0);
}