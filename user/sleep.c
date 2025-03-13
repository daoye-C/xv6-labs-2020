#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc,char **argv){
    
    if(argc<1){
        printf("usage: sleep<ticks>\n");
    }

    sleep(atoi(argv[0]));
    exit(0);

}