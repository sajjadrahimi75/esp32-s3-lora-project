#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/uart.h"

#include "uart_interrupt.h"


/* Global variables accessible from main.c */

char received_text[64] = "";
volatile int received_position = 0;
extern char SEND_MESSAGE[64];


/* Queue used by the ESP-IDF UART interrupt driver */

QueueHandle_t uart_interrupt_queue;



void uart_receive_task(void *parameter)
{
    uart_event_t event;
    char character;

    //(void)parameter;

    while (1)
    {
        /*
         * Wait until the UART hardware interrupt
         * creates an event.
         */
        if (xQueueReceive(uart_interrupt_queue,&event,portMAX_DELAY) == pdTRUE)
        {
            if (event.type == UART_DATA)
            {
                /*
                 * One event may contain one or more characters.
                 */
                
                    int length = uart_read_bytes(UART_NUM_0, &character, 1, portMAX_DELAY);

                    if (length == 1)
                    {
                        /*
                         * Ignore Enter characters.
                         */
                        if (character != '\r' && character != '\n'){
                            /*
                             * Add the new character to
                             * the previous characters.
                             */
                            if (received_position < 63)
                            {
                                received_text[received_position] =character;
                                received_position++;

                                /*
                                 * End the text with '\0'.
                                 */
                                received_text[received_position] =
                                    '\0';
                            }
                        }
                    }
            }
        }
    }
}


void uart_interrupt_init(void)
{
    uart_config_t configuration = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_NUM_0, &configuration);

    /*
     * This installs the ESP-IDF UART driver
     * and its internal hardware interrupt handler.
     */
    uart_driver_install( UART_NUM_0, 1024, 0, 10, &uart_interrupt_queue, 0);

    /*
     * Ask for a receive event when at least
     * one complete character is received.
     */
    uart_set_rx_full_threshold( UART_NUM_0, 1);

    /*
     * Start the task that processes UART events.
     */
    xTaskCreate(uart_receive_task,"uart_receive_task",2048,NULL,10,NULL);//firt null is related to parameter
    
}


void uart_send_message(void) //called from main.c
{
        uart_write_bytes(UART_NUM_0, SEND_MESSAGE, strlen(SEND_MESSAGE));

}