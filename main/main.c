#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "driver/gpio.h"
#include "uart_interrupt.h"
#include "lora_wrapper.h"

char SEND_MESSAGE[64]="";
extern char received_text[64];

/*
 * Message that we want to send by LoRa.
 */
char LORA_MESSAGE[64]="";

/*
 * Message received from LoRa.
 */
char LORA_RECEIVED_MESSAGE[64]="";

/*
 * 0 = idle
 * 1 = send LORA_MESSAGE
 * 2 = message received
 */
int LORA_STATUS = 0;



void app_main(void)
{


    /*
     * Store the result of LoRa functions.
     */
    int lora_result;

    /*
     * Store the number of received LoRa characters.
     */
    int lora_received_length;

    uart_interrupt_init();

    /*
     * Initialize LoRa.
     */
    lora_result = lora_init();

    gpio_reset_pin(GPIO_NUM_47);
    gpio_set_direction(GPIO_NUM_47, GPIO_MODE_OUTPUT);
    gpio_reset_pin(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_reset_pin(GPIO_NUM_7);
    gpio_set_direction(GPIO_NUM_7,GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_NUM_47, 1); //LED
    gpio_set_level(GPIO_NUM_4, 0);  //GPRS ON
    gpio_set_level(GPIO_NUM_7,0);   //GPRS RESET

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
         * LoRa send part.
         *
         * When LORA_STATUS is 1,
         * send the text inside LORA_MESSAGE.
         */
                /*
         * Prepare "Hello" to be sent every second.
         */
        
        strcpy(LORA_MESSAGE, "Hello");

        /*
         * Request LoRa transmission.
         */
        LORA_STATUS = 1;

        if (LORA_STATUS == 1)
        {
            /*
             * Send LORA_MESSAGE to lora_wrapper.cpp.
             */
            lora_result = lora_send(LORA_MESSAGE);

            /*
             * If lora_send returns 0,
             * sending was successful.
             */
            if (lora_result == 0)
            {
                /*
                 * Return LoRa status to idle.
                 */
                LORA_STATUS = 0;
            }
        }

        /*
         * LoRa receive part.
         *
         * Check whether a LoRa message was received.
         */
        lora_received_length = lora_receive(
            LORA_RECEIVED_MESSAGE,
            sizeof(LORA_RECEIVED_MESSAGE)
        );

        /*
         * If the returned value is bigger than 0,
         * a message was received.
         */
        if (lora_received_length > 0)
        {
            /*
             * Status 2 means a LoRa message
             * is available inside
             * LORA_RECEIVED_MESSAGE.
             */
            LORA_STATUS = 2;
        }

        /*
         * Wait one second.
         */
        
         gpio_set_level(GPIO_NUM_47, 1); //LED
        vTaskDelay(pdMS_TO_TICKS(2000));
         gpio_set_level(GPIO_NUM_47, 0); //LED
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}