#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

#define BUF_SIZE 512

/*
strchr(str,""):找到第一个与""相等的位置并返回以这个为卡头的字符串
*/
int main(int argc,char *argv[])
{
    char buf[BUF_SIZE+1]={0};               // 缓冲区
    char *origin_argv[MAXARG];              // 命令行
    uint bias=0;                            // 偏移量，用于看已经读取了多少命令行
    int stdin_end=0;                        // 看标准输入是否结束，结束为1

    for(int i=1;i<argc;i++)               // 读取,手动跳过xargs（我们自己要实现的）
        origin_argv[i-1]=argv[i];

    while(!(stdin_end&&bias==0))
    {
        uint remain_size=BUF_SIZE-bias;
        int read_bytes=read(0,buf+bias,remain_size);
        if(read_bytes<0)
        {
            fprintf(2,"read error\n");
            exit(1);
        }
        bias+=read_bytes;
        if(read_bytes==0)
            stdin_end=1;
        if(!stdin_end)
        {
            close(0);
            // printf("program end\n");
        }

        char *line_end=strchr(buf,'\n');
        while(line_end)
        {
            char xbuf[BUF_SIZE+1]={0};
            memcpy(xbuf,buf,line_end-buf);              // copy长度为第三个的字符
            origin_argv[argc-1]=xbuf;
            origin_argv[argc]=0;

            int pid=fork();
            if(pid==0)
            {
                if(!stdin_end)
                close(0);
                if(exec(argv[1],origin_argv)<0)
                {
                    fprintf(2,"exev failure\n");
                    exit(1);
                }
            }
            else
            {
                memmove(buf,line_end+1,BUF_SIZE-(line_end-buf)-1);        // 将\n后的输入移动到前面
                bias-=(line_end-buf)+1;
                memset(buf+bias,0,BUF_SIZE-bias);
                wait(0);
                line_end=strchr(buf,'\n');
            }
        }
    }
    exit(0);
}