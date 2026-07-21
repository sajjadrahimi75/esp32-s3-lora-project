#ifndef UART_INTERRUPT_H
#define UART_INTERRUPT_H

#include <stdbool.h>

extern  char received_text[64];
extern volatile int received_position;
extern char SEND_MESSAGE[64];



void uart_send_message(void);
void uart_interrupt_init(void);
void uart_send_text(const char *text);

#endif