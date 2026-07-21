#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "uart_interrupt.h"
#include "lora_wrapper.h"

char SEND_MESSAGE[64]="";
extern char received_text[64];

void app_main(void)
{
    uart_interrupt_init();

    while (1)
    {
        /*
         * Send all characters received until now.
         */
        if (received_position > 0)
        {
            strcat(received_text,"\n\r"); //adding \n\r to received_text
            strcpy(SEND_MESSAGE,received_text); //put received_text in send_message
            uart_send_message();  //jump to uart_interrupt.c
            strcpy(received_text,"");
        }
        else
        {
          
            strcpy(SEND_MESSAGE,"No character received\r\n");
            uart_send_message();  //jump to uart_interrupt.c
        }

        /*
         * Wait one second.
         */
        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}