// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKETSIZE 13
extern uint ticks;                // 全局时间

/*
一个哈希桶会对应多个buf
用时间戳代替LRU算法，可以防止操作整个bcache用一把锁，但是需要遍历整个桶来获取最小时间戳
对一个桶而言，如果申请时没有空闲，就在全局驱逐
*/
static int getidx(int blockno)
{
  return blockno%BUCKETSIZE;
}

// 1.修改bcache结构以适配要求，还有更改buf结构体
struct {
  // 哈希桶
  struct spinlock lock[BUCKETSIZE];
  struct spinlock evictlock;            // 全局锁，便于调试
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
  struct buf head[BUCKETSIZE];          // 桶的头节点
} bcache;

// 2.更改bcache的binit，初始化所有桶锁
void
binit(void)
{
  struct buf *b;

  for(int i=0;i<BUCKETSIZE;i++)
  {
    initlock(&bcache.lock[i], "bcache");            // 初始化桶锁
    bcache.head[i].next = &bcache.head[i];
  }
  initlock(&bcache.evictlock,"evictlock");          // 初始化全局锁
  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");            // 初始化buffer的睡眠锁
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
    b->next = bcache.head[0].next;                // 初始化链表，全在head[0]上
    if(bcache.head[0].next)
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
// 3.修改以寻找桶，驱逐桶，抢夺桶
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int idx = getidx(blockno);
  struct spinlock *lock = &bcache.lock[idx];              // 获取设备编号对应的桶锁
  acquire(lock);

  // Is the block already cached?
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->las = ticks;
      release(lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

// 未命中缓存时的驱逐,遍历全局获得las最大的buf
  release(lock);
  int min_las = 0x7fffffff;
  struct buf *pre = 0;
  int bucketidx = -1;
  for(int i=0;i<BUCKETSIZE;i++)
  {
    acquire(&bcache.lock[i]);
    int find_better = 0;
    for(b = &bcache.head[i];b -> next != &bcache.head[i];b = b->next)
    {
      if(b->next->refcnt==0 && min_las > b->next->las)
      {
        min_las=b->next->las;
        pre = b;
        find_better=1;
      }
    }

    if(find_better)                   // 找到更适合的驱逐块,释放之前块的锁
    {
      if(bucketidx!=-1)
      release(&bcache.lock[bucketidx]);
      bucketidx = i;                  // 改成当前
    }
    else release(&bcache.lock[i]);
  }
// 寻找到可以驱逐的buf，先把buf从原来的块释放，并插入当前快，然后二次检查，因为有可能其他CPU或者进程已经缓存了
struct buf *fbuf = 0;
if(pre!=0)
{
  fbuf = pre->next;
  pre->next=pre->next->next;
  release(&bcache.lock[bucketidx]);
}
if(fbuf)
{
  acquire(lock);
  fbuf->next = bcache.head[idx].next;
  bcache.head[idx].next=fbuf;
}

// 二次检查，是否已缓存
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->las = ticks;
      release(lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
if(!fbuf) panic("bget: no buffers");
// 到这里就是没缓存，直接返回fbuf
  fbuf->dev = dev;
  fbuf->blockno = blockno;
  fbuf->valid = 0;
  fbuf->refcnt = 1;
  fbuf->las = ticks;
  release(lock);
  acquiresleep(&fbuf->lock);
  return fbuf;
}

// Return a locked buf with the contents of the indicated block. 还未缓存磁盘的buf，先读磁盘
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.写磁盘
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
// 4.修改relse，只修改时间戳即可
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int idx = getidx(b->blockno);
  struct spinlock *lock = &bcache.lock[idx];
  acquire(lock);
  b->refcnt--;
  release(lock);
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  int idx = getidx(b->blockno);
  struct spinlock *lock = &bcache.lock[idx];
  acquire(lock);
  b->refcnt++;
  b->las = ticks;
  release(lock);
}

void
bunpin(struct buf *b) {
  int idx = getidx(b->blockno);
  struct spinlock *lock = &bcache.lock[idx];
  acquire(lock);
  b->refcnt--;
  release(lock);
}


