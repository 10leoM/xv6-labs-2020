#include <stdio.h>
#include <string.h>

char *basename(char *path)
{
  char *p;
  char *cur=strchr(path,'/');
  while(cur!=0)
  {
    p=cur+1;
    cur=strchr(cur+1,'/');
  }
  return p;
}

int main(int argc,char *argv[])
{
  char *path="os/xv6/lab1";
  char *cur=basename(path);
  int len=cur-path;
  char *next=path+1;
  printf("%d , %s\n",len,cur);
  printf("%d , %s\n",next-path,next);
}
