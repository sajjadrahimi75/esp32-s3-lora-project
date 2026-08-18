#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "lora_wrapper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
    int year   = 0;
    int month  = 0;
    int day    = 0;
    int hour   = 0;
    int minute = 0;
    int time_error=0;
    int counter=0;
    int second_count=0;
    char message[400] = "hello";
    char lora_received_message[200] = "";
    printf("Main started\n");

            while(1)
            {
             

            lora_wrapper_run(
            message,
            lora_received_message,
            &year,
            &month,
            &day,
            &hour,
            &minute,
            &time_error
            );

            printf("Back in main.c\n");

            printf("Date: %02d/%02d/%04d\n",
                day, month, year);

            printf("Time: %02d:%02d\n",
                hour, minute);

             counter++;
            snprintf(message, sizeof(message), "TESTING FOR %d//time error:%d//received message is:%s", counter,time_error,lora_received_message);

            printf("\n received message is=%s\r\n",lora_received_message);

                 second_count=600;
                 while(second_count>2)
                 {
                 gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
                 vTaskDelay(pdMS_TO_TICKS(500));
                 gpio_set_level(GPIO_NUM_47,1); //TURN OFF LED
                 vTaskDelay(pdMS_TO_TICKS(500));
                 second_count--;
                 }

                 
                 
            }   
}
    