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

#define NBUFBUCKETS (23)

struct buffers_bucket {
  struct spinlock lock;
  struct buf *bufs[NBUF];
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buffers_bucket buckets[NBUFBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for (int i = 0; i < NBUFBUCKETS; i++) {
    initlock(&bcache.buckets[i].lock, "bcache_bucket");
    for (int j = 0; j < NBUF; j++) bcache.buckets[i].bufs[j] = 0;
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++) {
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buffers_bucket *bucket = &bcache.buckets[blockno%NBUFBUCKETS];

  acquire(&bucket->lock);

  // Is the block already cached?
  for (int i = 0; i < NBUF; i++) {
    b = bucket->bufs[i];
    if(b && b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  acquire(&bcache.lock);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for (int i = 0; i < NBUF; i++) {
    b = &bcache.buf[i];
    if(b->refcnt == 0) {
      for (int j = 0; j < NBUF; j++) {
        if (!bucket->bufs[j] || bucket->bufs[j]->refcnt == 0) {
          if (bucket->bufs[j]) bucket->bufs[j]->dev = bucket->bufs[j]->blockno = 0;
          bucket->bufs[j] = b;
          break;
        }
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
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
  
  struct buffers_bucket *bucket = &bcache.buckets[b->blockno%NBUFBUCKETS];

  releasesleep(&b->lock);

  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}

void
bpin(struct buf *b) {
  struct buffers_bucket *bucket = &bcache.buckets[b->blockno%NBUFBUCKETS];

  acquire(&bucket->lock);
  b->refcnt++;
  release(&bucket->lock);
}

void
bunpin(struct buf *b) {
  struct buffers_bucket *bucket = &bcache.buckets[b->blockno%NBUFBUCKETS];

  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}


