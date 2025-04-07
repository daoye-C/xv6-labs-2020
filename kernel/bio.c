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

#define BUFMAPNUM 13 
#define bmaphash(blockno) (blockno % BUFMAPNUM)

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  
  struct buf bufmap[BUFMAPNUM];
  struct spinlock bufmap_lock[BUFMAPNUM];
} bcache;

void
binit(void)
{
  struct buf *b;

  char lockname[16];
  for(int i = 0; i < BUFMAPNUM; i++)
  {
    snprintf(lockname, sizeof(lockname), "bcache_%d", i);
    initlock(&bcache.bufmap_lock[i], lockname);
  }

  initlock(&bcache.lock, "bcache");

  //---lab8---
  // initialise 
  for(int i = 0; i < BUFMAPNUM; i ++)
  {
    bcache.bufmap[i].prev = &bcache.bufmap[i];
    bcache.bufmap[i].next = &bcache.bufmap[i];

  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b ++) // 全部都分配到0号桶去
  {
    b->next = bcache.bufmap[0].next;
    b->prev = &bcache.bufmap[0];
    initsleeplock(&b->lock, "buffer");
    bcache.bufmap[0].next->prev = b;
    bcache.bufmap[0].next = b;
    b->timestamp = 0;
  }


  //---lab8---
  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bid = bmaphash(blockno);
  acquire(&bcache.bufmap_lock[bid]);

  // Is the block already cached?
  for(b = bcache.bufmap[bid].next; b != &bcache.bufmap[bid]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;

      acquire(&tickslock);
      b->timestamp  = ticks;
      release(&tickslock);

      release(&bcache.bufmap_lock[bid]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  b = 0;
  struct buf *tmp;
  for(int i = bid, cycle = 0; cycle <= BUFMAPNUM; cycle ++, i = (i+1) % BUFMAPNUM ) // 遍历所有桶
  {
    if(i != bid)
    {
      if(!holding(&bcache.bufmap_lock[i]))
        acquire(&bcache.bufmap_lock[i]);
    }

    for(tmp = bcache.bufmap[i].next; tmp != &bcache.bufmap[i]; tmp = tmp->next)
    {
      if(tmp->refcnt == 0 && (b == 0 || tmp->timestamp < b->timestamp))
        b = tmp;
    }

    if(b)
    {
      if(i != bid) // 窃取自其他桶时
      {
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.bufmap_lock[i]);

        b->next = bcache.bufmap[bid].next;
        b->prev = &bcache.bufmap[bid];
        bcache.bufmap[bid].next->prev = b;
        bcache.bufmap[bid].next = b;
      }

      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.bufmap_lock[bid]);
      acquiresleep(&b->lock);
      return b;
    }
    else
    {
      if(i != bid)
        release(&bcache.bufmap_lock[i]);
    }
  }


  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
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

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int bid = bmaphash(b->blockno);
  releasesleep(&b->lock);


  acquire(&bcache.bufmap_lock[bid]);
  b->refcnt--;

  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);
  
  release(&bcache.bufmap_lock[bid]);
}

void
bpin(struct buf *b) {
  int bid = bmaphash(b->blockno);
  acquire(&bcache.bufmap_lock[bid]);
  b->refcnt++;
  release(&bcache.bufmap_lock[bid]);
}

void
bunpin(struct buf *b) {
  int bid = bmaphash(b->blockno);
  acquire(&bcache.bufmap_lock[bid]);
  b->refcnt--;
  release(&bcache.bufmap_lock[bid]);
}


