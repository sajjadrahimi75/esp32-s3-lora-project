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

//eeprom
#include "nvs_flash.h"
#include "nvs.h"

//#include "image_stream.h"


/*
 * If this function is already declared inside camera.h,
 * this declaration is still compatible.
 */
uint32_t camera_get_real_jpeg_size(void);
char wifi_message[300];

const char *wifi_message_text = NULL;

char polito_password[100]={0};
char polito_username[100]={0};
char polito_ssid[20]={0};
char wifi_hotspot_username[20]={0};
char wifi_hotspot_password[20]={0};

//const size_t test_image_len = sizeof(test_image);   // calculate bytes of image
 RTC_DATA_ATTR int photo_quality = 1 ;
 RTC_DATA_ATTR int photo_size = 21 ;
 RTC_DATA_ATTR int auto_focus = 0 ;
RTC_DATA_ATTR int sharpness=0;
RTC_DATA_ATTR int contrast=0;
RTC_DATA_ATTR int brightness=0;
RTC_DATA_ATTR int saturation=0;
RTC_DATA_ATTR int hmirror=1;
RTC_DATA_ATTR int vmirror=0;
 RTC_DATA_ATTR int wifi_try;
 int do_not_send_text=0;
 int photo_quality_primary=0;
 


void wifi_message_sending(void)
{
    esp_err_t err;

    for(int i=0;i<5;i++)
    {
        snprintf(wifi_message,sizeof(wifi_message),"%s",wifi_message_text);
        err=wifi_message_send();
        if (err==ESP_OK)
        {
           printf("\n\rmain.c:WIFI MESSAGE HAS BEEN SENT(number of try=%d)\n",i);
           break;
        }
        printf("\n\rmain.c:WIFI MESSAGE HAS NOT BEEN SENT(number of try=%d)\n",i);
    }
}

void wifi_turn_on(void)
{
    //wifi_select = 1; //1=polito**2=hotspot

    for(int i=0;i<10;i++)
    {
        esp_err_t err = wifi_connect_internal();

        if(err==ESP_OK)
        {
            printf("\n\rmain.c:CONNECTING TO WIFI(number of try=%d)\n",i);
            break;
        }
        printf("\n\r***main.c:FAIL TO CONNECTING TO WIFI(number of try=%d)\n",i);
        wifi_full_deinit();
        wifi_select++;
        printf("\n\r************main.c:INCREASE WIFI_SELECT\n");
        if(wifi_select>3)
        {
            wifi_select=1;
            printf("\n\r*********main.c:WIFI_SELECT RESETED\n");
        }
        
    }
    
    

}


void save_int_to_memory(const char *key, int value)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        "storage",
        NVS_READWRITE,
        &handle
    );

    if (err == ESP_OK)
    {
        nvs_set_i32(handle, key, value);

        nvs_commit(handle);

        nvs_close(handle);

        printf("Saved %s = %d\n", key, value);
    }
    else
    {
        printf("NVS open failed\n");
    }
}


void save_string_to_memory(const char *key, const char *value)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        "storage",
        NVS_READWRITE,
        &handle
    );

    if (err != ESP_OK)
    {
        printf(
            "NVS OPEN FAILED: %s\n",
            esp_err_to_name(err)
        );

        return;
    }


    err = nvs_set_str(
        handle,
        key,
        value
    );

    if (err != ESP_OK)
    {
        printf(
            "NVS SET STRING FAILED: %s\n",
            esp_err_to_name(err)
        );

        nvs_close(handle);

        return;
    }


    err = nvs_commit(handle);

    if (err != ESP_OK)
    {
        printf(
            "NVS COMMIT FAILED: %s\n",
            esp_err_to_name(err)
        );

        nvs_close(handle);

        return;
    }


    nvs_close(handle);


    printf(
        "NVS SAVED: %s = %s\n",
        key,
        value
    );
}


int load_int_from_memory(const char *key, int default_value)
{
    nvs_handle_t handle;

    int32_t value = default_value;

    esp_err_t err = nvs_open(
        "storage",
        NVS_READONLY,
        &handle
    );

    if (err == ESP_OK)
    {
        if (nvs_get_i32(handle, key, &value) != ESP_OK)
        {
            value = default_value;
        }

        nvs_close(handle);
    }

    return value;
}


void load_string_from_memory(
    const char *key,
    char *buffer,
    size_t buffer_size,
    const char *default_value)
{

    nvs_handle_t handle;

    strncpy(buffer, default_value, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';

    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK)
    {
        size_t required_size = buffer_size;

        nvs_get_str(
            handle,
            key,
            buffer,
            &required_size
        );

        nvs_close(handle);
    }
}


void app_main(void)
{
gpio_reset_pin(GPIO_NUM_0);
gpio_set_direction(GPIO_NUM_0,GPIO_MODE_INPUT);
gpio_pullup_en(GPIO_NUM_0); 
gpio_pulldown_dis(GPIO_NUM_0);  


///////////////////////////////////just for test
gpio_reset_pin(GPIO_NUM_4);
gpio_set_direction(GPIO_NUM_4,GPIO_MODE_OUTPUT);
gpio_reset_pin(GPIO_NUM_7);
gpio_set_direction(GPIO_NUM_7,GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_4,1);//GSM ON
printf("\nGSM TURNED ON");
vTaskDelay(pdMS_TO_TICKS(1500));
gpio_set_level(GPIO_NUM_7,0);//GSM RESET
printf("\nRESETING GSM");
vTaskDelay(pdMS_TO_TICKS(1200));
gpio_set_level(GPIO_NUM_7, 1);     // release RESET
printf("\nRELEASE RESET");
vTaskDelay(pdMS_TO_TICKS(4000)); 
///////////////////////////////////////////

//////////////////////////////////////////////////////




    /////////////////////////////eeprom
    esp_err_t nvs_err = nvs_flash_init();

    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    photo_quality_primary = load_int_from_memory("quality", 1);
    photo_size = load_int_from_memory("size", 21);
    auto_focus = load_int_from_memory("autofocus", 0);
    wifi_select =load_int_from_memory("wifiselect", 3);
    sharpness=load_int_from_memory("sharpness", 0);
    contrast=load_int_from_memory("contrast", 0);
    brightness=load_int_from_memory("brightness", 0);
    saturation=load_int_from_memory("saturation", 0);
    hmirror=load_int_from_memory("hmirror", 1);
    vmirror=load_int_from_memory("vmirror", 0);
    load_string_from_memory("polito_password",polito_password,sizeof(polito_password),"");
    load_string_from_memory("polito_username",polito_username,sizeof(polito_username),"");
    load_string_from_memory("polito_ssid",polito_ssid,sizeof(polito_ssid),"polito");
    load_string_from_memory("wifi_hotspot_username",wifi_hotspot_username,sizeof(wifi_hotspot_username),"");
    load_string_from_memory("wifi_hotspot_password",wifi_hotspot_password,sizeof(wifi_hotspot_password),"");

    if(wifi_select>3 || wifi_select<1)
    {
        wifi_select=3;
    }
    /////////////////////////////////////
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 60000,
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
//esp_err_t cam_err =camera_init_ov5640();

/*
        if (cam_err != ESP_OK)
        {
            printf("\nCAMERA INITIALIZATION FAILED\n");
            return;
        }
*/

//camera_take_photo();
//photo_size=21;
//photo_quality=2;
/*
turning wifi on
*/
    wifi_turn_on();

int take_photo=0;
snprintf(wifi_message,sizeof(wifi_message),
"DEVICE TURNED ON(LIST OF FUNCTIONS:afon/afoff/rst/p/q=/s=/wifi=/sharpness=/contrast=/brightness=/saturation=/hmirror=/vmirror=/politousername=/politopassword=/politossid=/wifiusername=/wifipassword=)");

wifi_message_send();

while(1)
{
    

    /////////////////////////////////////////////receive message
    printf("\n\r **********************main.c:SEARCHING FOR NEW MESSAGE\n");
    for(int i=0;i<3;i++)
    {
        esp_err_t err = wifi_receive();

        if (err == ESP_OK)
        {
            printf("main.c:Received message:\n");
            printf("%s\n", wifi_received_message);
            break;
        }
        if(err!=ESP_OK && i>1)
        {
            printf("main.c:WiFi receive failed\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            wifi_full_deinit();
            wifi_turn_on();
        }
    }

    if (strcmp(wifi_received_message, "afon") == 0)
    {
      auto_focus=1;
      printf("main.c:AUTO FOCUS TURNED ON\n");
      save_int_to_memory("autofocus", auto_focus);
      wifi_message_text="AUTO FOCUS TURNED ON";
      wifi_message_sending();
      wifi_received_message[0] = '\0';
      //snprintf(wifi_message,sizeof(wifi_message),"AUTO FOCUS TURNED ON");
      //wifi_message_send();

    }
        if (strcmp(wifi_received_message, "afoff") == 0)
    {
      auto_focus=0;
      printf("main.c:AUTO FOCUS TURNED OFF\n");
      save_int_to_memory("autofocus", auto_focus);
      wifi_message_text="AUTO FOCUS TURNED OFF";
      wifi_message_sending();
      wifi_received_message[0] = '\0';
    }
        if (strcmp(wifi_received_message, "rst") == 0)
    {
      photo_quality=1;
      photo_size=21;
      sharpness=0;
      contrast=0;
      brightness=0;
      saturation=0;
      save_int_to_memory("quality", photo_quality_primary);
      save_int_to_memory("size", photo_size);
      save_int_to_memory("sharpness", sharpness);
      save_int_to_memory("contrast", contrast);
      save_int_to_memory("brightness", brightness);
      save_int_to_memory("saturation", saturation);
      printf("main.c:ALL PARAMETER HAS BEEN RESETED\n");
      wifi_message_text="ALL PARAMETER HAS BEEN RESETED";
      wifi_message_sending();
      wifi_received_message[0] = '\0';
    }

    if (strcmp(wifi_received_message, "p") == 0)
    {
        printf("main.c:TAKING PHOTO\n");
        wifi_message_text="TAKING PHOTO";
        wifi_message_sending();
        wifi_received_message[0] = '\0';
        take_photo=1;
    }

    if (sscanf(wifi_received_message, "q=%d", &photo_quality_primary) == 1)
    {
        printf("main.c:PHOTO QUALITY = %d\n", photo_quality_primary);
        save_int_to_memory("quality", photo_quality_primary);
        snprintf(wifi_message,sizeof(wifi_message),"PHOTO QUALITY = %d",photo_quality_primary);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "s=%d", &photo_size) == 1)
    {
        printf("main.c:PHOTO SIZE = %d", photo_size);
        save_int_to_memory("size", photo_size);
        snprintf(wifi_message,sizeof(wifi_message),"PHOTO SIZE=%d",photo_size);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "wifi=%d", &wifi_select) == 1)
    {
        printf("main.c:WIFI SELECTION= %d->(1=POLITO/2=HOT SPOT/3=TEST,123456789) ", wifi_select);
        wifi_full_deinit();
        wifi_turn_on();
        save_int_to_memory("wifiselect", wifi_select);
        snprintf(wifi_message,sizeof(wifi_message),"WIFI SELECTION=%d",wifi_select);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "sharpness=%d", &sharpness) == 1)
    {
        if(sharpness>3)
        {
            sharpness=3;
        }
        if(sharpness<-3)
        {
            sharpness=-3;
        }
        printf("main.c:SHARPNESS= %d", sharpness);
        save_int_to_memory("sharpness", sharpness);
        snprintf(wifi_message,sizeof(wifi_message),"UPDATE SHARPNESS=%d",sharpness);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "contrast=%d", &contrast) == 1)
    {
        if(contrast>3)
        {
            contrast=3;
        }
        if(contrast<-3)
        {
            contrast=-3;
        }
        printf("main.c:contrast= %d", contrast);
        save_int_to_memory("contrast", contrast);
        snprintf(wifi_message,sizeof(wifi_message),"UPDATE CONTRAST=%d",contrast);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }
     
    if (sscanf(wifi_received_message, "brightness=%d", &brightness) == 1)
    {
        if(brightness>3)
        {
            brightness=3;
        }
        if(brightness<-3)
        {
            brightness=-3;
        }
        printf("main.c:brightness= %d", brightness);
        save_int_to_memory("brightness", brightness);
        snprintf(wifi_message,sizeof(wifi_message),"UPDATE BRIGHTNESS=%d",brightness);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "saturation=%d", &saturation) == 1)
    {
        if(saturation>4)
        {
            saturation=4;
        }
        if(saturation<-4)
        {
            saturation=-4;
        }
        printf("main.c:saturation= %d", saturation);
        save_int_to_memory("saturation", saturation);
        snprintf(wifi_message,sizeof(wifi_message),"UPDATE SATURATION=%d",saturation);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "hmirror=%d", &hmirror) == 1)
    {
        printf("main.c:hmirror= %d", hmirror);
        save_int_to_memory("hmirror",hmirror);
        snprintf(wifi_message,sizeof(wifi_message),"H-MIRROR=%d",hmirror);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "vmirror=%d", &vmirror) == 1)
    {
        printf("main.c:vmirror= %d", vmirror);
        save_int_to_memory("vmirror",vmirror);
        snprintf(wifi_message,sizeof(wifi_message),"V-MIRROR=%d",vmirror);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }
    

    if (sscanf(wifi_received_message, "politousername=%99s", polito_username) == 1)
    {
        printf("main.c:POLITO USERNAME=%s", polito_username);
        save_string_to_memory("polito_username", polito_username);
        snprintf(wifi_message,sizeof(wifi_message),"POLITO USERNAME=%s",polito_username);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

     if (sscanf(wifi_received_message, "politopassword=%99s", polito_password) == 1)
    {
        printf("main.c:POLITO PASSWORD=%s", polito_password);
        save_string_to_memory("polito_password", polito_password);
        snprintf(wifi_message,sizeof(wifi_message),"POLITO PASSWORD=%s",polito_password);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "politossid=%19s", polito_ssid) == 1)
    {
        printf("main.c:POLITO SSID=%s", polito_ssid);
        save_string_to_memory("polito_ssid", polito_ssid);
        snprintf(wifi_message,sizeof(wifi_message),"POLITO SSID=%s",polito_ssid);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "wifiusername=%19s", wifi_hotspot_username) == 1)
    {
        printf("main.c:WIFI SSID=%s", wifi_hotspot_username);
        save_string_to_memory("wifi_hotspot_username", wifi_hotspot_username);
        snprintf(wifi_message,sizeof(wifi_message),"WIFI SSID=%s",wifi_hotspot_username);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    if (sscanf(wifi_received_message, "wifipassword=%19s", wifi_hotspot_password) == 1)
    {
        printf("main.c:WIFI PASSWORD=%s", wifi_hotspot_password);
        save_string_to_memory("wifi_hotspot_password", wifi_hotspot_password);
        snprintf(wifi_message,sizeof(wifi_message),"WIFI PASSWORD=%s",wifi_hotspot_password);
        wifi_message_text=wifi_message;
        wifi_message_sending();
    }

    wifi_received_message[0] = '\0';

 
    ////////////////////////////////////////////////////////////////
    if(take_photo==1 || gpio_get_level(GPIO_NUM_0) == 0)
    {
           //////////////////////////////////////for test
            gpio_set_level(GPIO_NUM_4,0);//GSM OFF
            printf("\nGSM TURNED OFF");
            ///////////////////////////////////////////

        /*photo_quality++;
        if(photo_quality>3)
        {
            photo_quality=1;
            photo_size--;
            if(photo_size<1)
            { 
                printf("\n\rmain.c: FINISH TEST\n");
                while(1)
                    {
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    }
            }
        }*/
        int quality_try=0;
        photo_quality=photo_quality_primary;

        for(int i=0;i<21;i++)
        {
            camera_process();

            if(photo_capture_error==0)
            {
                printf("\n\r*******************PHOTO CAPTURED***********************\n");
                do_not_send_text=0;
                wifi_error=1;
                wifi_try=0;

                for(int i=0;i<5;i++)
                    { 
                        wifi_photo_send();
                        if (wifi_error==0)
                            {
                            break;
                            }
                        if(wifi_error==1)
                            {
                            printf("\n\rmain.c:PROBLEM TO SEND PHOTO WITH WIFI->TRY AGAIN\n");
                            wifi_try++;
                            vTaskDelay(pdMS_TO_TICKS(100));
                            }
                    }

                    snprintf(wifi_message,sizeof(wifi_message),
                    "Photo captured. Quality=%d, size=%d , size of PSRAM=%u bytes,wifi try=%d",
                    photo_quality,photo_size,(unsigned)photo_psram_size,wifi_try);
                    wifi_message_send();
                    take_photo=0;
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    
                break;
            }

            if(photo_capture_error==1)
            {
            printf("\n\r***************PHOTO HAS NOT CAPTURED*******************\n");
            quality_try++;
            if(quality_try>1)
            {
               photo_quality++;
               quality_try=0; 
            }
            
            }
        }

           //////////////////////////////////////for test
            gpio_set_level(GPIO_NUM_4,1);//GSM ON
            printf("\nGSM TURNED ON");
            vTaskDelay(pdMS_TO_TICKS(1500));
            gpio_set_level(GPIO_NUM_7,0);//GSM RESET
            printf("\nRESETING GSM");
            vTaskDelay(pdMS_TO_TICKS(1200));
            gpio_set_level(GPIO_NUM_7, 1);     // release RESET
            printf("\nRELEASE RESET");
            vTaskDelay(pdMS_TO_TICKS(4000)); 
            ///////////////////////////////////////////

    }
    vTaskDelay(pdMS_TO_TICKS(5000));

 
    
}
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
#if 0
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
#endif
 
        //sleep

        printf("\nSleeping for 15 minutes\n");


        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );


        esp_sleep_enable_timer_wakeup(10ULL * 60ULL * 1000000ULL);


        esp_deep_sleep_start();

    }
}

