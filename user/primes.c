#include "kernel/types.h"
#include "user/user.h"

#define READ 0
#define WRITE 1
// 父进程将非自己质数的倍数的值写入管道，子进程递归调用solve
void solve(int *pl)
{
    int pr[2];
    pipe(pr);
    int primeNum;
    if(read(pl[READ],&primeNum,sizeof(int))<sizeof(int))
    exit(0);

    printf("prime %d\n",primeNum);                   // 读取质数

    if(fork()==0)                                   // 子进程调用递归是为了和父进程同步（否则递归会先处理递归）
    {
        close(pr[WRITE]);
        solve(pr);
        exit(0);
    }
    else
    {
       int i;
       close(pr[READ]);
       while(read(pl[READ],&i,sizeof(int))!=0)
       {
            if(i%primeNum>0)
                write(pr[WRITE],&i,sizeof(int));
       }
       close(pr[WRITE]);        // 写结束
       close(pl[READ]);       // 左邻居读结束
       wait(0);
       exit(0);
    }
}

int main(int argc,char *argv[])
{
    int pl[2];
    pipe(pl);
    
    if(fork()==0)
    {
        close(pl[WRITE]);
        solve(pl);
        exit(0);
    }
    close(pl[READ]);
    for(int i=2;i<=35;i++)
    {
        write(pl[WRITE],&i,sizeof(int));
    }
    close(pl[WRITE]);
    wait(0);            // 等待子进程结束
    exit(0);
}