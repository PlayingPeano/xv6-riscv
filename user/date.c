#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int is_leap_year(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static const int days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

void convert_time(uint64 nanoseconds, int *y, int *mo, int *d, int *h, int *mi, int *s, int *ns) {
    *ns = nanoseconds % 1000000000;
    uint64 sec = nanoseconds / 1000000000;
    *s = sec % 60; sec /= 60;
    *mi = sec % 60; sec /= 60;
    *h = sec % 24; sec /= 24;

    int year = 1970;
    while (1) {
        int days = is_leap_year(year) ? 366 : 365;
        if (sec >= days) {
            sec -= days;
            year++;
        } else break;
    }
    *y = year;

    int month;
    for (month = 0; month < 12; month++) {
        int dim = days_in_month[month];
        if (month == 1 && is_leap_year(year)) dim++;
        if (sec >= dim) sec -= dim;
        else break;
    }
    *mo = month + 1;
    *d = sec + 1;
}

int main() {
    uint64 time;
    if (rtctime(&time) < 0) {
        fprintf(2, "date: failed to read RTC\n");
        exit(1);
    }

    int y, mo, d, h, mi, s, ns;
    convert_time(time, &y, &mo, &d, &h, &mi, &s, &ns);

    printf("%d-%d-%d %d:%d:%d.%d\n", y, mo, d, h, mi, s, ns);
    exit(0);
}