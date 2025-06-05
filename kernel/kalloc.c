// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PA2INDEX(pa) ((uint64)pa/PGSIZE)
int count[PHYSTOP/PGSIZE];                              // 物理页表全局计数器
void count_dec(uint64 pa);
void count_inc(uint64 pa);

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    count[PA2INDEX(p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  count_dec((uint64)pa);                                     // 计数=0才释放,否则只减少计数
  if(count[PA2INDEX(pa)]>0)
  {
    // printf("PA: %p dec, count[pa] = %d\n", count[PA2INDEX(pa)]);
    return;
  }
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  if(r)
  count[PA2INDEX((r))] = 1;                                   // 初始化计数
  release(&kmem.lock);
  return (void*)r;
}

void count_dec(uint64 pa)
{
  if (pa >= PHYSTOP) {
    panic("addref: pa too big");
  }
  acquire(&kmem.lock);
  count[PA2INDEX(pa)]--;
  release(&kmem.lock);
}
void count_inc(uint64 pa)
{
  if (pa >= PHYSTOP) {
    panic("addref: pa too big");
  }
  acquire(&kmem.lock);
  count[PA2INDEX(pa)]++;
  release(&kmem.lock);
}