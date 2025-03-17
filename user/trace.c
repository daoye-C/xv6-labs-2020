#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];

  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  if (trace(atoi(argv[1])) < 0) { // the trace here is a system call 
    fprintf(2, "%s: trace failed\n", argv[0]); 
    exit(1);
  }
  
  for(i = 2; i < argc && i < MAXARG; i++){ // 这是
    nargv[i-2] = argv[i];
  }
  exec(nargv[0], nargv);// 跟踪的命令 ，nargv读取的时候只会从第1个开始
  exit(0);
}


/* 系统调用trace  格式： 
  trace <mask> <命令> <命令的参数>

  |----------|

  参数1 参数2

*/