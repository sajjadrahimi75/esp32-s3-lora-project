#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "esp_attr.h"

#include "driver/gpio.h"
#include "lora_wrapper.h"
#include "gsm.h"
#include "camera.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_task_wdt.h"
#include "esp_sleep.h"
#include "esp_heap_caps.h"

#include "wifi.h"

//#include "image_stream.h"


/*
 * If this function is already declared inside camera.h,
 * this declaration is still compatible.
 */
uint32_t camera_get_real_jpeg_size(void);


//const size_t test_image_len = sizeof(test_image);   // calculate bytes of image
 RTC_DATA_ATTR int photo_quality = 1 ;
 RTC_DATA_ATTR int photo_size = 21 ;

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
    int time_error = 0;
    int counter = 0;
    int senderror = 0;

    char message[400] = "hello";
    char lora_received_message[200] = "";
   


    gpio_reset_pin(GPIO_NUM_47);
    gpio_set_direction(
        GPIO_NUM_47,
        GPIO_MODE_OUTPUT
    );


    gpio_reset_pin(GPIO_NUM_2);  //BATTRY ADC SWITCH
    gpio_set_direction(
        GPIO_NUM_2,
        GPIO_MODE_OUTPUT
    );
    gpio_set_level(
        GPIO_NUM_2,
        0
    );


    printf("\nMAIN STARTED\n");


    while(1)
    {
      printf("\n***********************************************\n");
#if 0
        //LORA TRANSMOTTER


        snprintf(
            message,
            sizeof(message),
            "TESTING FOR %d//time error:%d//send error:%d//received message is:%s->at%02d:%02d",
            counter,
            time_error,
            senderror,
            lora_received_message,
            hour,
            minute
        );

        counter++;


        /*
        lora_received_message[0] = '\0';

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
        );
        */


        printf("Back in main.c\n");


        ///////////////////////////////////to lowercase

        for (int i = 0;
             lora_received_message[i] != '\0';
             i++)
        {
            lora_received_message[i] =
                tolower(
                    (unsigned char)
                    lora_received_message[i]
                );
        }

        /////////////////////////////////////////////


        printf(
            "Date: %02d/%02d/%04d\n",
            day,
            month,
            year
        );


        printf(
            "Time: %02d:%02d\n",
            hour,
            minute
        );


        printf(
            "\n received message is=%s\r\n",
            lora_received_message
        );


        lora_received_message[0] = 'p';


        if(lora_received_message[0] == 'p')
        {
            printf("\nSENDING PHOTO\n");

            //gsm_init(test_image,test_image_len);
            //gsm_ppp_init(NULL, 0);
        }



        //////camera

        printf("\nCAMERA TEST START\n");


        /*
         * ==================================================
         * SHOW PSRAM BEFORE CAMERA INITIALIZATION
         * ==================================================
         */

        size_t psram_total_before =heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

        size_t psram_free_before =heap_caps_get_free_size(MALLOC_CAP_SPIRAM);


        printf("\n===== PSRAM BEFORE CAMERA =====\n");

        printf("TOTAL PSRAM = %u bytes\n",(unsigned)psram_total_before);

        printf("TOTAL PSRAM = %.2f MB\n",psram_total_before /(1024.0 * 1024.0));

        printf("FREE PSRAM  = %u bytes\n",(unsigned)psram_free_before);

        printf( "===============================\n");


#endif
        esp_err_t cam_err =camera_init_ov5640();

/*
        if (cam_err != ESP_OK)
        {
            printf("\nCAMERA INITIALIZATION FAILED\n");
            return;
        }
*/

camera_take_photo();

#if 0
        /*
         * ==================================================
         * SHOW PSRAM AFTER CAMERA FRAMEBUFFER ALLOCATION
         * ==================================================
         */

        size_t psram_total_after_init =
            heap_caps_get_total_size(
                MALLOC_CAP_SPIRAM
            );

        size_t psram_free_after_init =
            heap_caps_get_free_size(
                MALLOC_CAP_SPIRAM
            );


        printf(
            "\n===== PSRAM AFTER CAMERA INIT =====\n"
        );

        printf(
            "TOTAL PSRAM = %u bytes\n",
            (unsigned)psram_total_after_init
        );

        printf(
            "TOTAL PSRAM = %.2f MB\n",
            psram_total_after_init /
            (1024.0 * 1024.0)
        );

        printf(
            "FREE PSRAM = %u bytes\n",
            (unsigned)psram_free_after_init
        );

        printf(
            "PSRAM USED BY CAMERA/DRIVER = %u bytes\n",
            (unsigned)(
                psram_free_before -
                psram_free_after_init
            )
        );

        printf(
            "===================================\n"
        );



        printf("\nSENDING PHOTO\n");

#endif
        /*

        
 * ==================================================
 * START CAMERA -> DMA -> PSRAM CAPTURE
 * ==================================================
 */
#if 0
printf(
    "\nMEASURING ONE CAMERA FRAME...\n"
);


uint32_t camera_jpeg_size =
    camera_measure_jpeg_size();


if (camera_jpeg_size > 0)
{
    printf(
        "\n========================================\n"
    );

    printf(
        "OV5640 JPEG SIZE = %u bytes\n",
        (unsigned)camera_jpeg_size
    );

    printf(
        "========================================\n"
    );
}
else
{
    printf(
        "JPEG SIZE MEASUREMENT FAILED\n"
    );
}

#endif
camera_power_off();
#if 0

        esp_err_t capture_err =
            camera_start_stream_capture();


        if (capture_err != ESP_OK)
        {
            printf(
                "CAMERA CAPTURE START FAILED\n"
            );

            return;
        }

        printf("WAITING FOR ONE COMPLETE CAMERA FRAME...\n");


        int wait_counter = 0;


        while (!camera_measurement_done())
        {
            vTaskDelay(
                pdMS_TO_TICKS(20)
            );


            wait_counter++;


            if (wait_counter >= 500)
            {
                printf(
                    "CAMERA FRAME TIMEOUT\n"
                );

                break;
            }
        }

        if (camera_measurement_done())
        {
            uint32_t camera_jpeg_size =
                camera_get_real_jpeg_size();


            printf(
                "\n========================================\n"
            );

            printf(
                "OV5640 FRAME MEASUREMENT\n"
            );

            printf(
                "========================================\n"
            );

            printf(
                "FRAME SIZE SETTING : QSXGA\n"
            );

            printf(
                "JPEG QUALITY       : 5\n"
            );

            printf(
                "OV5640 JPEG SIZE   : %u bytes\n",
                (unsigned)camera_jpeg_size
            );

            printf(
                "========================================\n"
            );


            camera_power_off();
        }
        else
        {
            printf(
                "FRAME MEASUREMENT FAILED\n"
            );
        }


        printf(
            "WAITING FOR PHOTO TO BE STORED IN PSRAM...\n"
        );


        /*
        * Wait for:
        *
        * VSYNC #1 -> start
        * DMA blocks -> PSRAM
        * VSYNC #2 -> end
        */
        int wait_counter = 0;


        while (!camera_photo_ready())
        {
            vTaskDelay(
                pdMS_TO_TICKS(20)
            );


            wait_counter++;


            /*
            * 20 ms x 500 = 10 seconds timeout.
            */
            if (wait_counter >= 500)
            {
                printf(
                    "PHOTO CAPTURE TIMEOUT\n"
                );

                uint32_t ov5640_timeout_size =
                    camera_get_real_jpeg_size();

                printf(
                    "OV5640 REPORTED JPEG SIZE AT TIMEOUT = %u bytes\n",
                    (unsigned)ov5640_timeout_size
                );

                printf(
                    "PSRAM STORED SIZE AT TIMEOUT = %u bytes\n",
                    (unsigned)camera_photo_size()
                );

                break;
            }
        }



        if (camera_photo_ready())
        {
            uint8_t *photo_data =
                camera_photo_data();


            size_t psram_photo_size =
                camera_photo_size();


            size_t psram_photo_capacity =
                camera_photo_capacity();


            /*
            * Camera is still powered here,
            * so read OV5640 registers now.
            */
            uint32_t ov5640_photo_size =
                camera_get_real_jpeg_size();



            printf(
                "\n========================================\n"
            );

            printf(
                "PHOTO CAPTURE COMPLETE\n"
            );

            printf(
                "OV5640 REPORTED JPEG SIZE = %u bytes\n",
                (unsigned)ov5640_photo_size
            );

            printf(
                "PSRAM STORED PHOTO SIZE   = %u bytes\n",
                (unsigned)psram_photo_size
            );

            printf(
                "PSRAM BUFFER CAPACITY     = %u bytes\n",
                (unsigned)psram_photo_capacity
            );

            printf(
                "========================================\n"
            );


            /*
            * OV5640 size has already been read.
            * Camera can now be powered off.
            */
            camera_power_off();



            /*
            * ==================================================
            * ONLY NOW START WIFI
            * ==================================================
            */

            printf(
                "\nPHOTO READY - NOW STARTING WIFI\n"
            );


            esp_err_t wifi_err =
                wifi_stream_start();


            if (wifi_err != ESP_OK)
            {
                printf(
                    "WIFI STREAM START FAILED: %s\n",
                    esp_err_to_name(wifi_err)
                );
            }
            else
            {
                printf(
                    "SENDING %u BYTES FROM PSRAM TO WIFI\n",
                    (unsigned)psram_photo_size
                );


                wifi_err =
                    wifi_stream_write(
                        photo_data,
                        psram_photo_size
                    );


                if (wifi_err != ESP_OK)
                {
                    printf(
                        "WIFI JPEG WRITE FAILED: %s\n",
                        esp_err_to_name(wifi_err)
                    );
                }
                else
                {
                    wifi_err =
                        wifi_stream_finish();


                    if (wifi_err == ESP_OK)
                    {
                        printf(
                            "PHOTO SENT TO NTFY\n"
                        );
                    }
                    else
                    {
                        printf(
                            "WIFI STREAM FINISH FAILED: %s\n",
                            esp_err_to_name(wifi_err)
                        );
                    }
                }
            }
        }
        else
        {
            printf(
                "\nPHOTO WAS NOT COMPLETED\n"
            );
        }
#endif

//printf("\n\rPHOTO QAULITY IS=%d",photo_quality);
 photo_quality++;
 if(photo_quality>6)
 {
    photo_quality=1;
    photo_size--;
    if(photo_size<1)
     { 
        printf("\n\r FINISH TEST\n");
        while(1)
            {
            vTaskDelay(pdMS_TO_TICKS(5000));
            }
     }
 }
 
        //sleep

        //printf("\nSleeping for 15 minutes\n");


        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );

#if 0
        esp_sleep_enable_timer_wakeup(
            1ULL * 60ULL * 1000000ULL
        );


        esp_deep_sleep_start();
#endif
    }
}