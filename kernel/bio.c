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

#define NBUCKETS 13  // hash bucket数量

struct {
  struct spinlock lock[NBUCKETS]; // 每个bucket一个lock
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS];  // head.next是当前bucket的MRU
} bcache;

int
hash(uint n) // hash函数
{
  return n%NBUCKETS;
}

void
binit(void)
{
  struct buf *b;

  for(int i=0;i<NBUCKETS;i++)
  {
    initlock(&bcache.lock[i], "bcache"); //将hash表中的每个lock初始化
    bcache.head[i].prev = &bcache.head[i];    // 仍然将每个bucket的头节点都指向自己
    bcache.head[i].next = &bcache.head[i];
  }
    // 此时因为buffer没有和磁盘块对应起来，所以blockno全部为初始值0，将其全部放在第一个bucket中
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int id=hash(blockno);
  acquire(&bcache.lock[id]);

  // Is the block already cached?
  // 首先在blockno对应的bucket中查找
  for(b = bcache.head[id].next; b != &bcache.head[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 如果在对应的bucket中没有找到，需要在其他bucket中找
  // 此时原来bucket的锁还不能释放，因为在其他bucket中找到buffer后，
  // 还要将其插到原来的bucket中
  // j表示表示下一个要探索的bucket，当它重新变成id，
  // 说明所有的buffer都在使用中，此时panic
  for(int j=hash(blockno+1);j!=id;j=(j+1)%NBUCKETS)
  {
    acquire(&bcache.lock[j]); // 获取当前bucket的锁
    for(b = bcache.head[j].prev; b != &bcache.head[j]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // 从当前bucket的链表中断开
        b->next->prev=b->prev;
        b->prev->next=b->next;
        release(&bcache.lock[j]);
        // 插到对应的bucket中去
        b->next=bcache.head[id].next;
        b->prev=&bcache.head[id];
        bcache.head[id].next->prev=b;
        bcache.head[id].next=b;
        release(&bcache.lock[id]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    // 如果当前bucket里没有找到，在转到下一个bucket之前，要释放当前bucket的锁
    release(&bcache.lock[j]);
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

  releasesleep(&b->lock);
  int id=hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    // 将b从原来的位置取下来放在链表开头
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[id].next;
    b->prev = &bcache.head[id];
    bcache.head[id].next->prev = b;
    bcache.head[id].next = b;
  }
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id=hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id=hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}