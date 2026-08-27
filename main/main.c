#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "lora_wrapper.h"
#include "gsm.h"
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
    int senderror=0;
    char message[400] = "hello";
    char lora_received_message[200] = "";
    char check_lora_income[3]="";
gpio_reset_pin(GPIO_NUM_47);
gpio_set_direction(GPIO_NUM_47,GPIO_MODE_OUTPUT);


printf("Main started\n");

gsm_init();
/*while(1)
{
    
//GSM 


//LORA TRANSMOTTER
printf("Back in main.c\n");

printf("Date: %02d/%02d/%04d\n",
    day, month, year);

printf("Time: %02d:%02d\n",
    hour, minute);

    
//snprintf(message, sizeof(message), "TESTING FOR %d//time error:%d//send error:%d//received message is:%s->at%02d:%02d", counter,time_error,senderror,check_lora_income,hour, minute);
counter++;
printf("\n received message is=%s\r\n",check_lora_income);

        second_count=900;
        while(second_count>2)
        {
        gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(GPIO_NUM_47,1); //TURN OFF LED
        vTaskDelay(pdMS_TO_TICKS(500));
        second_count--;
        }

lora_received_message[0] = '\0';

    lora_wrapper_run(
    message,
    lora_received_message,
    check_lora_income,
    &year,
    &month,
    &day,
    &hour,
    &minute,
    &time_error,
    &senderror
    ); 

    
        
} */  
}
    