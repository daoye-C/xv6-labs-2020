// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};



struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU]; // kmem -> kmem[NCPU]  lab8

void
kinit()
{
  char lockname[8];
  for(int i = 0; i < NCPU; i++)
  {
    // 这里需要补充下命名的问题  虽然这样也可以通过
    snprintf(lockname, sizeof(lockname), "kmem_%d", i);
    initlock(&kmem[i].lock, lockname);
  }

  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");



  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int id = cpuid();

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int id = cpuid();

  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if(r)
  {
    kmem[id].freelist = r->next;
  }
  else // r 如果不存在则需要向其他cpu空闲列表进行窃取
  {
    for(int i = 0; i < NCPU; i++)
    {
      if(id == i) continue;
      acquire(&kmem[i].lock);
      struct run *r_stolen = kmem[i].freelist;
      if(r_stolen)
      {
        kmem[i].freelist = r_stolen->next;
        release(&kmem[i].lock);
        r = r_stolen;
        break;
      }
      else 
      {
        release(&kmem[i].lock);
        continue;
      }
    }
  }
  release(&kmem[id].lock);
  pop_off();




  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
