#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "driver/gpio.h"
#include "lora_wrapper.h"
#include "gsm.h"
#include "camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_sleep.h"

//const size_t test_image_len = sizeof(test_image);   // calculate bytes of image


void app_main(void)
{
    esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 600000,
    .idle_core_mask = (1 << 0) | (1 << 1),
    .trigger_panic = false
     };

ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&wdt_config));

    int year   = 0;
    int month  = 0;
    int day    = 0;
    int hour   = 0;
    int minute = 0;
    int time_error=0;
    int counter=0;
    int senderror=0;
    char message[400] = "hello";
    char lora_received_message[200] = "";
   
gpio_reset_pin(GPIO_NUM_47);
gpio_set_direction(GPIO_NUM_47,GPIO_MODE_OUTPUT);

gpio_reset_pin(GPIO_NUM_2);  //BATTRY ADC SWITCH
gpio_set_direction(GPIO_NUM_2,GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_2,0);



printf("Main started\n");


while(1)
{ 
    //vTaskDelay(pdMS_TO_TICKS(3000));
    

//LORA TRANSMOTTER

    
snprintf(message, sizeof(message), "TESTING FOR %d//time error:%d//send error:%d//received message is:%s->at%02d:%02d", counter,time_error,senderror,lora_received_message,hour, minute);
counter++;

/*lora_received_message[0] = '\0';

    lora_wrapper_run(
    message,
    lora_received_message,
    &year,
    &month,
    &day,
    &hour,
    &minute,
    &time_error,
    &senderror
    ); */

    printf("Back in main.c\n");
///////////////////////////////////to lowercase
    for (int i = 0; lora_received_message[i] != '\0'; i++)
    {
    lora_received_message[i] = tolower((unsigned char)lora_received_message[i]);
    }
/////////////////////////////////////////////

    printf("Date: %02d/%02d/%04d\n",day, month, year);

    printf("Time: %02d:%02d\n",hour, minute);

    printf("\n received message is=%s\r\n",lora_received_message);
    lora_received_message[0]='p';
    if(lora_received_message[0]=='p')
    {
    printf("\nSENDING PHOTO\n");
    //gsm_init(test_image,test_image_len);
   //gsm_ppp_init(NULL, 0);
    
    }




    //////camera
   printf("\nCAMERA TEST START\n");

esp_err_t cam_err = camera_init_ov5640();

if (cam_err != ESP_OK)
{
    printf("\nCAMERA INITIALIZATION FAILED\n");
    return;
}

printf("\nSENDING PHOTO\n");

camera_fb_t *fb = camera_take_photo();

if (fb != NULL)
{
    printf("ESP JPEG first: %02X %02X %02X %02X\n",
       fb->buf[0], fb->buf[1], fb->buf[2], fb->buf[3]);

printf("ESP JPEG last : %02X %02X %02X %02X\n",
       fb->buf[fb->len - 4],
       fb->buf[fb->len - 3],
       fb->buf[fb->len - 2],
       fb->buf[fb->len - 1]);

    printf("\nPHOTO CAPTURED\n");
    printf("JPEG SIZE = %u bytes\n", (unsigned)fb->len);

    camera_send_photo_uart(fb);

    camera_return_photo(fb);
}
else
{
    printf("\nPHOTO CAPTURE FAILED\n");
}
    

    //sleep
    printf("\nSleeping for 15 minutes\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_sleep_enable_timer_wakeup(5ULL * 60ULL * 1000000ULL);
    esp_deep_sleep_start();
    
    
    
    
    

    

    
        
}   
}
    