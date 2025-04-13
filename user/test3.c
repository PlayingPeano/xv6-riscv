#include "kernel/types.h"
#include "user/user.h"

void
test_rw()
{
  int m = mutex();
  char buf[1];
  printf("Test read/write:\n");
  if (read(m, buf, 1) < 0) {
    printf("  Read: OK\n");
  } else {
    printf("  Read: FAIL\n");
  }
  if (write(m, "X", 1) < 0) {
    printf("  Write: OK\n");
  } else {
    printf("  Write: FAIL\n");
  }
  close(m);
}

void
test_close_locked()
{
  printf("\nTest close locked mutex:\n");
  int m = mutex();
  mutex_lock(m);

  if (close(m) == 0) {
    printf("  Self-close: OK\n");
  } else {
    printf("  Self-close: FAIL\n");
  }
  m = mutex();
  mutex_lock(m);
  if (fork() == 0) {
    if (close(m) >= 0) {
      printf("  Other-close: OK\n");
    }
    exit(0);
  }
  wait(0);
  close(m);
}

void
test_exit()
{
  printf("\nTest process exit:\n");
  int m = mutex();
  if (fork() == 0) {
    mutex_lock(m);
    exit(0);
  }
  wait(0);
  if (mutex_lock(m) == 0) {
    printf("  Mutex unlocked: OK\n");
    mutex_unlock(m);
  } else {
    printf("  Mutex still locked: FAIL\n");
  }
  close(m);
}

void
test_unlock_foreign()
{
  printf("\nTest foreign unlock:\n");
  int m = mutex();
  mutex_lock(m);
  if (fork() == 0) {
    if (mutex_unlock(m) < 0) {
      printf("  Unlock failed: OK\n");
    } else {
      printf("  Unlock succeeded: FAIL\n");
    }
    exit(0);
  }
  wait(0);
  mutex_unlock(m);
  close(m);
}

void
test_lock_unlock()
{
  int m = mutex();
  if (mutex_lock(m) == 0) {
    printf("mutex locked successfully\n");
  } else {
    printf("lock failed\n");
    exit(0);
  }
  
  if (mutex_unlock(m) == 0) {
    printf("mutex unlocked successfully\n");
  } else {
    printf("unlock failed\n");
    exit(0);
  }
  close(m);
}

int
main()
{
  test_rw();
  test_close_locked();
  test_exit();
  test_unlock_foreign();
  test_lock_unlock();
  exit(0);
}