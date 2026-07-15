#include <arch/timer.h>
#include <kernel/panic.h>
#include <types.h>

#define TIMER_FREQ 10000000
#define SIE_STIE (1 << 5)

u64 timer_read()
{
	u64 time;
    asm volatile("csrr %0, time" : "=r"(time));
    return time;
}

void timer_irq_enable()
{
	u64 mask = SIE_STIE;
    asm volatile("csrs sie, %0" :: "r"(mask));
}

void timer_irq_disable()
{
	u64 mask = SIE_STIE;
    asm volatile("csrc sie, %0" :: "r"(mask));
}

void timer_set_alarm(u64 secs)
{
	u64 now = timer_read();
    u64 alarm_time = now + (secs * TIMER_FREQ);
    asm volatile("csrw stimecmp, %0" :: "r"(alarm_time));
}

void timer_irq()
{
	u64 disable_val = -1ULL;
    asm volatile("csrw stimecmp, %0" :: "r"(disable_val));
}
