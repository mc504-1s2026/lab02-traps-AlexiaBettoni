#include <kernel/serial.h>
#include <kernel/panic.h>
#include <types.h>

#define SERIAL_BASE 0x10000000

#define UART_REG(reg) ((volatile u8 *)(SERIAL_BASE + (reg)))

#define UART_RBR UART_REG(0) // leitura
#define UART_THR UART_REG(0) // escrita
#define UART_IER UART_REG(1)
#define UART_FCR UART_REG(2) // FIFO 
#define UART_LCR UART_REG(3) // Line Control 
#define UART_LSR UART_REG(5) // Line Status

#define UART_LSR_DR   (1 << 0) 
#define UART_LSR_TEMT (1 << 5) 
#define RX_BUF_SIZE 256

static char rx_buffer[RX_BUF_SIZE];
static volatile int rx_head = 0;
static volatile int rx_tail = 0;

static struct spinlock serial_lock;

void serial_init()
{
	spin_init(&serial_lock);
	
	*UART_IER = 0x00;

    *UART_LCR = 0x80;

    *UART_REG(0) = 0x03;
    *UART_REG(1) = 0x00;

    *UART_LCR = 0x03;

    *UART_FCR = 0x07;
}

void serial_irq_enable()
{
	*UART_IER |= 0x01;
}

void serial_irq_disable()
{
	*UART_IER &= ~0x01;
}

void serial_irq()
{
	spin_lock(&serial_lock);
	while (*UART_LSR & UART_LSR_DR) {
        char c = *UART_RBR;
        int next_head = (rx_head + 1) % RX_BUF_SIZE;
        if (next_head != rx_tail) {
            rx_buffer[rx_head] = c;
            rx_head = next_head;
        }
    }
	spin_unlock(&serial_lock);
}

size_t serial_read(char *buf)
{
	size_t count = 0;
	
	spin_lock(&serial_lock);
    while (rx_tail != rx_head) {
        buf[count++] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    }
    
	spin_unlock(&serial_lock);
    return count;
}

void serial_puts(char *str)
{
	while (*str) {
        serial_putc(*str++);
    }
}

void serial_putc(char c)
{
	while (!(*UART_LSR & UART_LSR_TEMT));
    *UART_THR = c;
}
