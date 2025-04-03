#include "kernel/types.h"
#include "user/user.h"

void
test_unsync()
{
  printf("=== Without mutex ===\n");
  int pid = fork();
  if (pid == 0) {
    for (int i = 1; i <= 5; i++) {
      for (char c = 'a'; c <= 'z'; c++) {
        printf("%d: arg %d, char '%c'\n", getpid(), i, c);
      }
    }
    exit(0);
  } else {
    for (int i = 1; i <= 5; i++) {
      for (char c = 'A'; c <= 'Z'; c++) {
        printf("%d: arg %d, char '%c'\n", getpid(), i, c);
      }
    }
    wait(0);
  }
}

void
test_sync()
{
  printf("\n=== With mutex ===\n");
  int m = mutex();
  int pid = fork();
  if (pid == 0) {
    for (int i = 1; i <= 5; i++) {
      for (char c = 'a'; c <= 'z'; c++) {
        mutex_lock(m);
        printf("%d: arg %d, char '%c'\n", getpid(), i, c);
        mutex_unlock(m);
      }
    }
    exit(0);
  } else {
    for (int i = 1; i <= 5; i++) {
      for (char c = 'A'; c <= 'Z'; c++) {
        mutex_lock(m);
        printf("%d: arg %d, char '%c'\n", getpid(), i, c);
        mutex_unlock(m);
      }
    }
    wait(0);
    close(m);
  }
}

int
main()
{
  test_unsync();
  test_sync();
  exit(0);
}