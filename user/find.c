#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

/*
strcmp(s1,s2):相同返回0，s1>s2返回正数，s1<s2返回负数
strchr(s,c):查找s中第一次出现c的位置，返回从这个位置开始的后面的字符串
strrchr(s,c):查找s中最后一次出现c的位置，返回从这个位置开始的后面的字符串
fstat(fd,*stat):从fd中读取到stat
stat(path，*stat):从路径path读取到stat
*/
char *basename(char *path)
{
    char *p="";
    char *cur=strchr(path,'/');
    while(cur!=0)
    {
        p=cur+1;
        cur=strchr(cur+1,'/');
    }
    return p;
}

void find(char *path,char *target)
{
    char buf[512],*p="";
    int fd;
    struct stat st;
    struct dirent de;

    if((fd=open(path,0))<0)                                             // 打开fd
    {
        //fprintf(2,"find: can not open %s\n",path);
        exit(0);
    }

    if(stat(path,&st)<0)
    {

        //fprintf(2,"find: cannot stat %s\n",path);
        close(fd);
        exit(0);
    }

    switch(st.type)
    {
        case T_FILE:;
        if(strcmp(basename(path),target)==0)
            printf("%s\n",path);
        /*
        else
        fprintf(2,"find: can not find %s\n",path);
        */
        break;


        case T_DIR:;
        if(strlen(path)+DIRSIZ+2>sizeof(buf))
        {
            //fprintf(2,"find: path too long\n");
            break;
        }
        memset(buf,0,sizeof(buf));
        uint len=strlen(path);
        memcpy(buf,path,len);
        p=buf+len;
        *p++='/';
        while(read(fd,&de,sizeof(de))==sizeof(de))
        {
            if(de.inum==0||strcmp(de.name,".")==0||strcmp(de.name,"..")==0)
            continue;
            
            memcpy(p,de.name,DIRSIZ);           // 内存上的copy
            p[DIRSIZ]=0;
            find(buf,target);
        }
        break;
    }
    close(fd);
    return;
}

int main(int argc,char*argv[])
{
    if(argc<3)
    {
        fprintf(2,"find need more instrument\n");
        exit(0);
    }
    char *path=argv[1];
    char *target=argv[2];
    find(path,target);
    exit(0);
}