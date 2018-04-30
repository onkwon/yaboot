#include "uart.h"
#include "bsp.h"

void uart_init()
{
	RCC_APB2ENR |= 1 << RCC_APB2ENR_IOPAEN; // gpioa clock enable
	GPIOA_CRH |= 0x0BUL << 4; // tx: out push-pull, PA9
	GPIOA_CRH |= 0x04UL << 8; // rx: in floating, PA10
	RCC_APB2ENR |= 1 << RCC_APB2ENR_USART1EN; // usart1 clock enable
	USART1_BRR = 8000000L/115200L;
	USART1_CR1 |= (1 << USART_RE) | (1 << USART_TE); // tx, rx enable
	USART1_CR1 |= 1 << USART_UE; // usart enable
}

int uart_put(int c)
{
	while (!(USART1_SR & (1 << USART_TXE)));
	USART1_DR = c & 0xff;
	return c;
}

int uart_get()
{
	while (!(USART1_SR & (1 << USART_RXNE)));
	return (int)(USART1_DR & 0xff);
}

void uart_puts(const char *s)
{
	while (s && *s)
		uart_put(*s++);
}
