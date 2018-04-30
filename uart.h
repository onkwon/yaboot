#ifndef __UART_H__
#define __UART_H__

enum {
	USART_RE = 2,
	USART_TE = 3,
	USART_UE = 13,
	USART_RXNE = 5, /* SR */
	USART_TXE = 7, /* SR */
};

enum {
	RCC_APB2ENR_IOPAEN	= 2,
	RCC_APB2ENR_USART1EN	= 14,
};

void uart_init();
int uart_put(int c);
int uart_get();
void uart_puts(const char *s);

#endif
