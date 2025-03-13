#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAX_ARG_LEN 512
#define stdin 0
#define stdout 1
#define stderr 2

int
main(int argc, char ** argv)
{
    int pid, buf_index = 0;
    char buf, arg[MAX_ARG_LEN], *args[MAXARG];

    if(argc < 2)
    {
        fprintf(stderr, "usage: xargs <command> ··· \n");
        exit(0);
    }


    //传入自带参数-命令
    
    for(int i = 1; i < argc ; i ++)
        args[i-1] = argv[i];

    while(read(stdin, &buf, 1) > 0)
    {
        if(buf == '\n')
        {
            arg[buf_index] = 0;//记得终止参数 否则会一直往后读  此时 buf_index指向一个未赋值的位置，当前参数结束应该赋值 0  
            if((pid = fork()) == 0)
            {
                args[argc - 1] = arg;
                args[argc] = 0;
                exec(args[0], args);
            }
            else
            {
                wait(0);
                buf_index = 0;
            }
        }
        else
        {
            arg[buf_index ++] = buf;
        }
    }
    exit(0);
}


/*

该函数有两种参数，一个来自于命令行，一个来自于自己的参数，需要将命令行传入的参数作为

自身的参也就是命令的参数。然后执行！


*/