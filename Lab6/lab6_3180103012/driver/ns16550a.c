#include "device.h"
#include "rinux_driver.h"

enum {
    UART_RBR = 0x00, /* Receive Buffer Register */
    UART_THR = 0x00, /* Transmit Hold Register */
    UART_IER = 0x01, /* Interrupt Enable Register */
    UART_DLL = 0x00, /* Divisor LSB (LCR_DLAB) */
    UART_DLM = 0x01, /* Divisor MSB (LCR_DLAB) */
    UART_FCR = 0x02, /* FIFO Control Register */
    UART_LCR = 0x03, /* Line Control Register */
    UART_MCR = 0x04, /* Modem Control Register */
    UART_LSR = 0x05, /* Line Status Register */
    UART_MSR = 0x06, /* Modem Status Register */
    UART_SCR = 0x07, /* Scratch Register */

    UART_LCR_DLAB = 0x80, /* Divisor Latch Bit */
    UART_LCR_8BIT = 0x03, /* 8-bit */
    UART_LCR_PODD = 0x08, /* Parity Odd */

    UART_LSR_DA = 0x01, /* Data Available */
    UART_LSR_OE = 0x02, /* Overrun Error */
    UART_LSR_PE = 0x04, /* Parity Error */
    UART_LSR_FE = 0x08, /* Framing Error */
    UART_LSR_BI = 0x10, /* Break indicator */
    UART_LSR_RE = 0x20, /* THR is empty */
    UART_LSR_RI = 0x40, /* THR is empty and line is idle */
    UART_LSR_EF = 0x80, /* Erroneous data in FIFO */
};

static volatile unsigned char *uart;

static void ns16550a_init() {
    uart                   = (unsigned char *)(void *)PA2VA(get_device_addr(UART_MMIO));
    unsigned int uart_freq = 1843200;
    unsigned int baud_rate = 115200;
    unsigned int divisor   = uart_freq / (16 * baud_rate);
    uart[UART_LCR]         = UART_LCR_DLAB;
    uart[UART_DLL]         = divisor & 0xff;
    uart[UART_DLM]         = (divisor >> 8) & 0xff;
    uart[UART_LCR]         = UART_LCR_PODD | UART_LCR_8BIT;
}

static char ns16550a_getchar() {
    if (uart[UART_LSR] & UART_LSR_DA) {
        return uart[UART_RBR];
    } else {
        return -1;
    }
}

static void ns16550a_putchar(char ch) {
    while ((uart[UART_LSR] & UART_LSR_RE) == 0)
        ;
    uart[UART_THR] = ch;
}

console_device_t console_ns16550a = {ns16550a_init, ns16550a_getchar, ns16550a_putchar};
