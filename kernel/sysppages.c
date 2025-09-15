#include "types.h"
#include "riscv.h"
#include "stdarg.h"
#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "proc.h"

const int PAGETABLE_SIZE = 512;
const int HEX_BASE = 16;

void
print_hex_padded(uint64 val)
{
  char hex_form[6];
  hex_form[0] = '0';
  hex_form[1] = 'x';
  for (int i = 2; i >= 0; --i) {
    int t = val % HEX_BASE;
    val /= HEX_BASE;
    if (t < 10) {
      hex_form[i + 2] = (char)(t + '0');
    } else {
      hex_form[i + 2] = (char)('a' + (t - 10));
    }
  }
  hex_form[5] = '\0';
  printf("%s", hex_form);
}

void
print_pte_flags(pte_t pte)
{
  printf("%s", (pte & PTE_R) ? "R" : "_");
  printf("%s", (pte & PTE_W) ? "W" : "_");
  printf("%s", (pte & PTE_X) ? "X" : "_");
  printf("%s", (pte & PTE_U) ? "U" : "_");
  printf("%s", (pte & PTE_G) ? "G" : "_");
  printf("%s", (pte & PTE_A) ? "A" : "_");
  printf("%s", (pte & PTE_D) ? "D" : "_");
}

uint64
ppages(uint64 p, int len, int mask, int change_flags_mode)
{
  pagetable_t pt2;
  if (!(pt2 = myproc()->pagetable)) {
    return -1;
  }
  
  if (mask > 3) {
    return -1;
  }
  mask <<= 6;
  
  if (p && len) {
    for (uint64 buffpgaddr = PGROUNDDOWN(p); buffpgaddr < PGROUNDUP(p + len);
         buffpgaddr += PGSIZE) {
      if (!walkaddr(pt2, buffpgaddr)) {
        return -1;
      }
    }
  }

  int l2_begin = 0;
  int l1_begin = 0;
  int l0_begin = 0;

  int l2_end = PAGETABLE_SIZE;
  int l1_end = PAGETABLE_SIZE;
  int l0_end = PAGETABLE_SIZE;

  if (p && len) {
    l2_begin = PX(2, p);
    l1_begin = PX(1, p);
    l0_begin = PX(0, p);

    l2_end = PX(2, p + len) + 1;
    l1_end = PX(1, p + len) + 1;
    l0_end = PX(0, p + len) + 1;
  }
  if (!change_flags_mode) {
    printf("PAGETABLE %p\n", (void*)(*pt2));
  }
  for (int l2 = l2_begin; l2 < l2_end; ++l2) {
    pte_t* pte2 = &pt2[l2];
    if (!(*pte2 & PTE_V)) {
      continue;
    }
    int l2_print_flag = 1;
    uint64 child2 = PTE2PA(*pte2);
    pagetable_t pt1 = (pagetable_t)child2;

    for (int l1 = l1_begin; l1 < l1_end; ++l1) {
      pte_t* pte1 = &pt1[l1];
      if (!(*pte1 & PTE_V)) {
        continue;
      }
      int l1_print_flag = 1;
      uint64 child1 = PTE2PA(*pte1);
      pagetable_t pt0 = (pagetable_t)child1;

      for (int l0 = l0_begin; l0 < l0_end; ++l0) {
        pte_t* pte0 = &pt0[l0];
        if (!(*pte0 & PTE_V)) {
          continue;
        }
        if (change_flags_mode) {
          *pte0 &= ~mask;
          continue;
        } else if ((*pte0 & mask) != mask) {
          continue;
        }

        if (l2_print_flag) {
          print_hex_padded(l2);
          printf(" -> %p ", (void*)(PTE2PA(*pte2)));
          print_pte_flags(*pte2);
          printf("\n");
          l2_print_flag = 0;
        }
        if (l1_print_flag) {
          printf(".........");
          print_hex_padded(l1);
          printf(" -> %p ", (void*)(PTE2PA(*pte1)));
          print_pte_flags(*pte1);
          printf("\n");
          l1_print_flag = 0;
        }
        printf("..................");
        print_hex_padded(l0);
        printf(" -> %p ", (void*)(PTE2PA(*pte0)));
        print_pte_flags(*pte0);
        printf("\n");
      }
    }
  }
  return 0;
}

uint64
sys_ppages(void)
{
  uint64 p;
  int len;
  int mask; // let: 00 = __; 01 = _A; 10 = D_; 11 = DA

  argaddr(0, &p);
  argint(1, &len);
  argint(2, &mask);

  return ppages(p, len, mask, 0);
}

uint64
sys_mppages(void)
{
  uint64 p;
  int len;
  int mask; // let: 00 = __; 01 = _A; 10 = D_; 11 = DA

  argaddr(0, &p);
  argint(1, &len);
  argint(2, &mask);

  return ppages(p, len, mask, 1);
}
