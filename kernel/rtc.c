#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "spinlock.h"

struct spinlock rtclock;
void 
rtc_init() 
{
  initlock(&rtclock, "rtc");
}

uint32
rtc_read_low()
{
  return *(volatile uint32*)RTC_LOW;
}

uint32
rtc_read_high()
{
  return *(volatile uint32*)RTC_HIGH;
}

uint64
rtc_get_time()
{
  uint32 low, high;
  acquire(&rtclock);
  do 
  {
    low = rtc_read_low();
    high = rtc_read_high();
  } while (rtc_read_high() != high);
  release(&rtclock);
  return ((uint64)high << 32) | low;
}