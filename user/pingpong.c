#include "kernel/types.h"
#include "user/user.h"


int main(int argc,char *argv[])

{
  char buf[10]={};
  int pipe1[2];
  int pipe2[2];
  pipe(pipe1);
  pipe(pipe2);

  int pid=fork();
  
  if(pid==0)
  {
    int tpid;
    close(pipe2[0]);
    close(pipe1[1]);
    read(pipe1[0],buf,1);

    tpid=getpid();
    printf("%d: received ping\n",tpid);
    
    write(pipe2[1],buf,1);
    close(pipe1[0]);
    close(pipe2[1]);
    exit(0);
  }
  else
  {
    int tpid;
    close(pipe1[0]);
    close(pipe2[1]);
    write(pipe1[1],buf,1);      // 先写，让子进程得到ping
    read(pipe2[0],buf,1);

    tpid=getpid();
    printf("%d: received pong\n",tpid);

    close(pipe1[1]);
    close(pipe2[0]);
    exit(0);
    }
    exit(0);
}
