#include <kernel/trap.h>
#include <kernel/panic.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

#define SIE_SEIE (1 << 9)

void handle_irq()
{
	u64 scause;
	asm volatile("csrr %0, scause" : "=r"(scause));

	u64 irq_code = scause & ~(1ULL << 63);

	switch (irq_code)
	{
	case 5:
		timer_irq();
		break;
	case 9:
		u32 irq = plic_hart_claim_irq(0);

		if (irq == 10){ 
			serial_irq();
		}

		if (irq != 0){
			plic_hart_complete_irq(0, irq);
		}
		break;
	default:
		panic("Problema não tratado: %d\n", irq_code);
	}
}

void handle_exception()
{
	u64 scause;
	u64 stval;
	asm volatile("csrr %0, scause" : "=r"(scause));
	asm volatile("csrr %0, stval" : "=r"(stval));
	if (scause == 12 || scause == 13 || scause == 15)
	{
		panic("Page fault: %d", scause);
	}
	panic("Problema não tratado: %d", scause)
}

void trap_setup()
{
	u64 entry = (u64)trap_entry;
	asm volatile("csrw stvec, %0" ::"r"(entry));

	u64 mask = SIE_SEIE;
    asm volatile("csrs sie, %0" :: "r"(mask));
}

void handle_trap()
{
	u64 scause;
	asm volatile("csrr %0, scause" : "=r"(scause));

	if (scause & (1ULL << 63)) {
		handle_irq();
	} else {
		handle_exception();
	}
}

void hart_irq_enable()
{
	u64 mask = 1 << 1;
	asm volatile("csrs sstatus, %0" ::"r"(mask));
}

u64 hart_irq_save()
{
	u64 sstatus;
	u64 mask = 1 << 1;
	asm volatile("csrr %0, sstatus" : "=r"(sstatus));
	asm volatile("csrc sstatus, %0" ::"r"(mask));
	return sstatus;
}

void hart_irq_restore(u64 flags)
{
	asm volatile("csrw sstatus, %0" ::"r"(flags));
}

void hart_irq_disable()
{
	u64 mask = 1 << 1;
	asm volatile("csrc sstatus, %0" ::"r"(mask));
}
