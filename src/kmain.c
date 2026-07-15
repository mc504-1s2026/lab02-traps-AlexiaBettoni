#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <arch/plic.h>
#include <kernel/string.h>

#define CMD_BUFFER_SIZE 128
static char cmd_buf[CMD_BUFFER_SIZE];
static int cmd_idx = 0;

static volatile u64 alarm_trigger_time = 0;
static volatile int alarm_pending = 0;

extern int _hartid[];

void handle_alarm_check() {
    if (alarm_pending && timer_read() >= alarm_trigger_time) {
        serial_puts("\r\nalarm\r\n> ");
        for (int i = 0; i < cmd_idx; i++) {
            serial_putc(cmd_buf[i]);
        }
        alarm_pending = 0;
    }
}

void execute_command(char *line) {
    if (strlen(line) == 0) {
        return;
    }

    if (strcmp(line, "uptime") == 0) {
        u64 secs = timer_read() / 10000000;
        info("%llds\n", secs);
    } 
    else if (strncmp(line, "echo ", 5) == 0) {
        info("%s\n", line + 5);
    } 
    else if (strncmp(line, "alarm ", 6) == 0) {
        char *arg = line + 6;
        u64 secs = 0;
        while (*arg >= '0' && *arg <= '9') {
            secs = secs * 10 + (*arg - '0');
            arg++;
        }
        
        if (secs > 0) {
            alarm_trigger_time = timer_read() + (secs * 10000000);
            alarm_pending = 1;
        }
    } 
    else {
        info("Unknown command: %s\n", line);
    }
}

void kmain()
{
    printk_set_level(LOG_DEBUG);
    info("S-mode\n");
    info("hart %d\n", _hartid[0]);
    info("arrumando memoria virtual...\n");
    vm_init();

    info("traps...\n");
    trap_setup();
    info("timer...\n");
    timer_irq_enable();
    info("serial...\n");
    serial_init();
    serial_irq_enable();

    plic_irq_set_priority(10, 1);
    plic_hart_enable_irq(0, 10);
    plic_hart_set_threshold(0, 0);

    hart_irq_enable();

    info("shell...\n");
    serial_puts("> ");

    while (1) {
        char incoming[16];
        size_t n = serial_read(incoming);

        for (size_t i = 0; i < n; i++) {
            char c = incoming[i];

            if (c == '\r') {
                serial_puts("\r\n");
                cmd_buf[cmd_idx] = '\0';
                execute_command(cmd_buf);
                cmd_idx = 0;
                serial_puts("> ");
            } 
            else if (c == 127 || c == '\b') {
                if (cmd_idx > 0) {
                    cmd_idx--;
                    serial_puts("\b \b");
                }
            } 
            else if (cmd_idx < (CMD_BUFFER_SIZE - 1)) {
                cmd_buf[cmd_idx++] = c;
                serial_putc(c);
            }
        }

        handle_alarm_check();
    }
}