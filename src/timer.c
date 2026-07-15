#include <arch/timer.h>
#include <kernel/panic.h>

#define TIMER_FREQ 10000000
#define SIE_STIE (1 << 5)

u64 timer_read()
{
	u64 time_val;
    __asm__ __volatile__("csrr %0, time" : "=r"(time_val));
    return time_val;
}

void timer_irq_enable()
{
	u64 mask = SIE_STIE;
    __asm__ __volatile__("csrs sie, %0" :: "r"(mask));
}

void timer_irq_disable()
{
	u64 mask = SIE_STIE;
    __asm__ __volatile__("csrc sie, %0" :: "r"(mask));
}

void timer_set_alarm(u64 secs)
{
	u64 now = timer_read();
    u64 alarm_time = now + (secs * TIMER_FREQ);
    __asm__ __volatile__("csrw stimecmp, %0" :: "r"(alarm_time));
}

void timer_irq()
{
	u64 disable_val = -1ULL;
    __asm__ __volatile__("csrw stimecmp, %0" :: "r"(disable_val));
}
