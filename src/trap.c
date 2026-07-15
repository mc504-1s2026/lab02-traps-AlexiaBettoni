#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/serial.h>
#include <arch/timer.h>
#include <arch/plic.h>

#define SIE_SEIE (1 << 9)

extern void trap_entry();

void handle_irq()
{
	u64 scause;
	__asm__ __volatile__("csrr %0, scause" : "=r"(scause));

	u64 irq_code = scause & ~(1ULL << 63);

	switch (irq_code)
	{
	case 5:
		timer_irq();
		break;
	case 9:{
		u32 irq = plic_hart_claim_irq(0);

		if (irq == 10){ 
			serial_irq();
		}

		if (irq != 0){
			plic_hart_complete_irq(0, irq);
		}
		break;
	}
	default:
		panic("Problema não tratado: %d\n", irq_code);
	}
}

void handle_exception()
{
	u64 scause;
	u64 stval;
	__asm__ __volatile__("csrr %0, scause" : "=r"(scause));
	__asm__ __volatile__("csrr %0, stval" : "=r"(stval));
	if (scause == 12 || scause == 13 || scause == 15)
	{
		panic("Page fault: %d", scause);
	}
	panic("Problema não tratado: %d", scause);
}

void trap_setup()
{
	u64 entry = (u64)trap_entry;
	__asm__ __volatile__("csrw stvec, %0" ::"r"(entry));

	u64 mask = SIE_SEIE;
    __asm__ __volatile__("csrs sie, %0" :: "r"(mask));
}

void handle_trap()
{
	u64 scause;
	__asm__ __volatile__("csrr %0, scause" : "=r"(scause));

	if (scause & (1ULL << 63)) {
		handle_irq();
	} else {
		handle_exception();
	}
}

void hart_irq_enable()
{
	u64 mask = 1 << 1;
	__asm__ __volatile__("csrs sstatus, %0" ::"r"(mask));
}

u64 hart_irq_save()
{
	u64 sstatus;
	u64 mask = 1 << 1;
	__asm__ __volatile__("csrr %0, sstatus" : "=r"(sstatus));
	__asm__ __volatile__("csrc sstatus, %0" ::"r"(mask));
	return sstatus;
}

void hart_irq_restore(u64 flags)
{
	__asm__ __volatile__("csrw sstatus, %0" ::"r"(flags));
}

void hart_irq_disable()
{
	u64 mask = 1 << 1;
	__asm__ __volatile__("csrc sstatus, %0" ::"r"(mask));
}
