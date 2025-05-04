#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"

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
  do {
    low = rtc_read_low();
    high = rtc_read_high();
  } while (rtc_read_high() != high);
  return ((uint64)high << 32ull) | low;
}