#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "stat.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "param.h"
#include "proc.h"

struct file*
mutexalloc(void)
{
  struct file *f;
  struct sleeplock *sl;

  if((f = filealloc()) == 0)
    return 0;
  if((sl = (struct sleeplock*)kalloc()) == 0){
    fileclose(f);
    return 0;
  }
  initsleeplock(sl, "mutex");
  f->type = FD_MUTEX;
  f->readable = 0;
  f->writable = 0;
  f->mutex = sl;
  printf("mutex allocated: %p\n", sl);
  return f;
}

void
mutexclose(struct file *f)
{
  printf("mutex closed: %p\n", f->mutex);
  kfree((char*)f->mutex);
  f->mutex = 0;
}