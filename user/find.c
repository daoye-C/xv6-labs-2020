#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


#define stdin 0
#define stdout 1
#define stderr 2
void 
find(char* path, char *filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
  
    if((fd = open(path, 0)) < 0){
      fprintf(stderr, "ls: cannot open %s\n", path);
      return;
    }
  
    if(fstat(fd, &st) < 0){//将文件信息解析进st结构体
      fprintf(stderr, "ls: cannot stat %s\n", path);
      close(fd);
      return;
    }
  
    switch(st.type){
    case T_FILE:
        if(strcmp(path + strlen(path) - strlen(filename), filename) == 0)// match
            printf("%s\n",path);
        break;
  
    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("ls: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';//加上 / 进入下一层级
        while(read(fd, &de, sizeof(de)) == sizeof(de)){//在文件夹中文件的排列方式就是一个一个de
            if(de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);//函数作用，将de.name复制到p中进行（de.name大小就是DIRSIZ）

            p[DIRSIZ] = 0;//加入一个结尾的标志

            if(stat(buf, &st) < 0){//前面的行为是在buf的后面拼接，实际上完成了对每一个文件名的拼接，然后进行访问
                printf("ls: cannot stat %s\n", buf);
                continue;
            }
            if(strcmp(buf + strlen(buf) - 2, "/.") != 0 && strcmp(buf + strlen(buf) - 3, "/..") != 0)
                find(buf, filename);
        }
        break;
    }
    close(fd);
}

int main(int argc, char ** argv)
{
    if(argc < 3) 
    {
        printf("usage: find path filename\n");
        exit(0);
    }
    char target[512];
    target[0] = '/';
    strcpy(target+1, argv[2]);
    find(argv[1], argv[2]);
    exit(0);
}


