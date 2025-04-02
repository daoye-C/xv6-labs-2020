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

//-----lab6-----
#define PA2IDX(pa) (((uint64)pa - KERNBASE)/PGSIZE)
#define REF(pa) ref.count[PA2IDX(pa)] 

struct ref_stru
{
  struct spinlock lock;
  int count[PA2IDX(PHYSTOP)+1];
}ref;


//-----lab6-----

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
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

  acquire(&ref.lock);
  if(--REF(pa) <= 0)
  {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&ref.lock);

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
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    REF(r) = 1;
  }
  return (void*)r;
}


///////// lab6 /////
void*
refkalloc(void* pa) // 物理复制 无关映射
{
  acquire(&ref.lock);
  if(REF(pa) <= 1)
  {
    release(&ref.lock);
    return pa;
  }

  uint64 mem = (uint64)kalloc();
  if(mem == 0)
  {
    release(&ref.lock);
    return 0;
  }
  memmove((void*)mem, (void*)pa, PGSIZE);

  REF(pa) -- ;

  release(&ref.lock);
  return (void*)mem;
}

void
ref_add(void* pa)
{
  acquire(&ref.lock);
  ++REF(pa);
  release(&ref.lock);
}

///////// lab6 /////