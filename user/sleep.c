#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char *argv[])
{
    if(argc!=2)
    {
        fprintf(2,"Usage: sleep <ticks>\n");     // 2输出到标准错误
        exit(1);
    }
    int tick=atoi(argv[1]);
    int res=sleep(tick);
    exit(res);
}