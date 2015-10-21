#ifndef UART_DEFINED
#define UART_DEFINED
#include <stdint.h>

#define BUFF_SIZE 128

typedef struct{
	uint8_t mfsch;
	uint8_t locno;
	uint16_t	txri, txwi;
	uint8_t		txbuff[BUFF_SIZE];
	uint16_t	rxri, rxwi;
	uint8_t		rxbuff[BUFF_SIZE];
} UARTFIFO;

void uart_init (volatile UARTFIFO *, uint32_t);
int uart_test (volatile UARTFIFO *);
void uart_putc (volatile UARTFIFO *, uint8_t);
uint8_t uart_getc (volatile UARTFIFO *);
uint8_t uart_received(volatile UARTFIFO *);
void uart_flush(volatile UARTFIFO *);
void uart_detach_buffer(volatile UARTFIFO *);
void uart_close(volatile UARTFIFO *);

#endif
