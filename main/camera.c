#include "camera.h"
#include <string.h>
#include "esp_heap_caps.h"
#include <stdio.h>
#include <stdint.h>

#include "esp_camera.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera_af.h"

static const char *TAG = "CAMERA";

int photo_capture_error = 1;
uint8_t *photo_psram_address = NULL;
size_t photo_psram_size = 0;
int camera_log=1;
static camera_fb_t *saved_fb = NULL;



/*
 * =========================================================
 * CAMERA PINS
 * =========================================================
 */

#define CAM_POWER_PIN   3

#define CAM_PIN_PWDN    16
#define CAM_PIN_RESET   -1

#define CAM_PIN_XCLK    48

#define CAM_PIN_SIOD    6
#define CAM_PIN_SIOC    5

#define CAM_PIN_VSYNC   15
#define CAM_PIN_HREF    19
#define CAM_PIN_PCLK    42

#define CAM_PIN_D0      40
#define CAM_PIN_D1      38
#define CAM_PIN_D2      37
#define CAM_PIN_D3      39
#define CAM_PIN_D4      41
#define CAM_PIN_D5      36
#define CAM_PIN_D6      35
#define CAM_PIN_D7      20




static void camera_prepare_psram_for_new_photo(void)
{
    /*
     * اگر از عکس قبلی framebuffer باقی مانده،
     * اول آن را به camera driver برگردان.
     */
    if (saved_fb != NULL)
    {
        esp_camera_fb_return(saved_fb);
        saved_fb = NULL;
    }

    /*
     * pointerهای عکس قبلی دیگر معتبر نیستند.
     */
    photo_psram_address = NULL;
    photo_psram_size = 0;

    /*
     * منابع camera driver قبلی را آزاد کن.
     */
    esp_camera_deinit();

    vTaskDelay(pdMS_TO_TICKS(100));

    /*
     * تمام PSRAM آزاد را صفر کن.
     */
    size_t free_psram =
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    size_t largest_block =
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    ESP_LOGI(
        TAG,
        "Free PSRAM before clear = %u bytes",
        (unsigned)free_psram
    );

    ESP_LOGI(
        TAG,
        "Largest PSRAM block = %u bytes",
        (unsigned)largest_block
    );

    if (largest_block > 0)
    {
        uint8_t *clear_buffer = heap_caps_malloc(
            largest_block,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
        );

        if (clear_buffer != NULL)
        {
            memset(clear_buffer, 0, largest_block);

            ESP_LOGI(
                TAG,
                "PSRAM cleared = %u bytes",
                (unsigned)largest_block
            );

            heap_caps_free(clear_buffer);
            clear_buffer = NULL;
        }
        else
        {
            ESP_LOGE(
                TAG,
                "Could not allocate PSRAM for clearing"
            );
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}

/*
 * =========================================================
 * CAMERA INITIALIZATION
 * =========================================================
 */

esp_err_t camera_process(void)
{
    //static esp_err_t camera_init_ov5640(void);

    photo_capture_error = 1;

    camera_prepare_psram_for_new_photo();

    
        /*
        * -----------------------------------------------------
        * POWER ON CAMERA
        * -----------------------------------------------------
        *
        * P-channel high-side MOSFET:
        *
        * GPIO LOW  -> Camera ON
        * GPIO HIGH -> Camera OFF
        */

        gpio_reset_pin(CAM_POWER_PIN);
        gpio_set_direction(CAM_POWER_PIN, GPIO_MODE_OUTPUT);

        /* Force camera OFF first */
        gpio_set_level(CAM_POWER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));

        /* Then turn camera ON */
        gpio_set_level(CAM_POWER_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(200));

        /*
        * Give camera power some time to stabilize.
        */
        vTaskDelay(pdMS_TO_TICKS(100));


        /*
        * -----------------------------------------------------
        * STANDARD ESP32 CAMERA CONFIGURATION
        * -----------------------------------------------------
        */

        camera_config_t config = {

            .pin_pwdn = CAM_PIN_PWDN,
            .pin_reset = CAM_PIN_RESET,

            .pin_xclk = CAM_PIN_XCLK,

            .pin_sccb_sda = CAM_PIN_SIOD,
            .pin_sccb_scl = CAM_PIN_SIOC,

            .pin_d0 = CAM_PIN_D0,
            .pin_d1 = CAM_PIN_D1,
            .pin_d2 = CAM_PIN_D2,
            .pin_d3 = CAM_PIN_D3,
            .pin_d4 = CAM_PIN_D4,
            .pin_d5 = CAM_PIN_D5,
            .pin_d6 = CAM_PIN_D6,
            .pin_d7 = CAM_PIN_D7,

            .pin_vsync = CAM_PIN_VSYNC,
            .pin_href = CAM_PIN_HREF,
            .pin_pclk = CAM_PIN_PCLK,


            /*
            * Camera XCLK.
            */
            .xclk_freq_hz = 10000000,


            .ledc_timer = LEDC_TIMER_0,
            .ledc_channel = LEDC_CHANNEL_0,


            /*
            * JPEG output.
            */
            .pixel_format = PIXFORMAT_JPEG, //PIXFORMAT_RGB565


            /*
            * OV5640 maximum resolution:
            * 2592 x 1944
            */
            .frame_size = photo_size,//from 1 to 21(biggest size)


            /*
            * JPEG quality is stored in RTC memory
            * and controlled from main.c.
            */
            .jpeg_quality = photo_quality,


            /*
            * For now use ONE framebuffer.
            *
            * Do not use 2 while debugging.
            */
            .fb_count = 1,
            //idf.py monitor 2>&1 | Tee-Object -FilePath QSXGA_Q5.txt

            /*
            * Wait until framebuffer is available.
            */
            .grab_mode = CAMERA_GRAB_WHEN_EMPTY,


            /*
            * IMPORTANT:
            *
            * QSXGA JPEG should use PSRAM.
            *
            * Do NOT use CAMERA_FB_IN_DRAM here.
            */
            .fb_location = CAMERA_FB_IN_PSRAM,
        };


        if (camera_log==1){ESP_LOGI(TAG,"Initializing OV5640...");}

  
        /*
        * -----------------------------------------------------
        * INITIALIZE ORIGINAL ESP32-CAMERA DRIVER
        * -----------------------------------------------------
        */

        esp_err_t err =esp_camera_init(&config);


        if (err != ESP_OK)
        {
            if (camera_log == 1){ESP_LOGE(TAG, "Camera init failed: 0x%x", err);}

            gpio_set_level(CAM_POWER_PIN, 1);

            photo_capture_error = 1;

            return err;
        }

    



        /*
        * -----------------------------------------------------
        * GET CAMERA SENSOR
        * -----------------------------------------------------
        */

        sensor_t *sensor = esp_camera_sensor_get();

        if (sensor == NULL)
        {
            ESP_LOGE(TAG, "Camera sensor not available");

            esp_camera_deinit();

            gpio_set_level(CAM_POWER_PIN, 1);

            photo_psram_address = NULL;
            photo_psram_size = 0;
            photo_capture_error = 1;

            vTaskDelay(pdMS_TO_TICKS(100));

            return ESP_FAIL;
        }
       /*
        * -----------------------------------------------------
        * BASIC IMAGE SETTINGS
        * -----------------------------------------------------
        */
        int pll_result = sensor->set_pll(
            sensor,
            0,//0
            120,//120
            4,//4
            0,//0
            2,//2
            2,//2
            1,//1
            6//6
        );

        if (camera_log==1){ESP_LOGI(TAG, "set_pll result = %d", pll_result);}

        int reg3824 = sensor->get_reg(sensor, 0x3824, 0xFF);

        if (camera_log==1){ESP_LOGI(TAG,
                "REG 0x3824 = 0x%02X, PCLK divider = %d",
                reg3824,
                reg3824 & 0x1F);}

        sensor->set_sharpness(sensor,sharpness);//0

        sensor->set_contrast(sensor,contrast);//0

        sensor->set_brightness(sensor,brightness);//0

        sensor->set_saturation(sensor,saturation);//0


        sensor->set_whitebal(sensor,1);

        sensor->set_exposure_ctrl(sensor,1);

        sensor->set_gain_ctrl(sensor,1);

        sensor->set_colorbar(sensor,0);

        sensor->set_hmirror(sensor,hmirror);//1
        sensor->set_vflip(sensor,vmirror);//0

        vTaskDelay(pdMS_TO_TICKS(200));
        
        //sensor->set_reg(sensor, 0x3821, 0x06, 0x06);
       /* for (int i = 0; i < 3; i++)
        {
            camera_fb_t *tmp = esp_camera_fb_get();

            if (tmp)
            {
                esp_camera_fb_return(tmp);
            }
        }*/
        
 /*
 * =========================================================
 * AUTOFOCUS INITIALIZATION + DEBUG
 * =========================================================
 */  bool autofocus_ready = false;
        if(auto_focus==1)
        {
           


            /*
            * Check whether driver reports AF support
            */
            if (camera_log==1){ESP_LOGI(
                TAG,
                "AF supported = %d",
                esp_camera_af_is_supported(sensor)
            );}


            /*
            * Read AF status register BEFORE initialization
            */
            int af_status_before =
                sensor->get_reg(
                    sensor,
                    0x3029,
                    0xFF
                );

            if (camera_log==1){ESP_LOGI(
                TAG,
                "AF STATUS BEFORE INIT = 0x%02X",
                af_status_before
            );}


            /*
            * Autofocus configuration
            */
            esp_camera_af_config_t af_cfg = {
                .mode = ESP_CAMERA_AF_MODE_MANUAL,
                .timeout_ms = 20000,
            };


            if (camera_log==1){ESP_LOGI(
                TAG,
                "AF init starting..."
            );}


            /*
            * Initialize autofocus firmware
            */
            err = esp_camera_af_init(
                sensor,
                &af_cfg
            );


            if (camera_log==1){ESP_LOGI(
                TAG,
                "AF init returned: %s",
                esp_err_to_name(err)
            );}


            /*
            * Read AF status register AFTER initialization
            */
            int af_status_after =
                sensor->get_reg(
                    sensor,
                    0x3029,
                    0xFF
                );

            if (camera_log==1){ESP_LOGI(
                TAG,
                "AF STATUS AFTER INIT = 0x%02X",
                af_status_after
            );}


            /*
            * Check result
            */
            if (err != ESP_OK)
            {
                if (camera_log==1){ESP_LOGE(
                    TAG,
                    "Autofocus init failed"
                );}
            }
            else
            {
                autofocus_ready = true;

                if (camera_log==1){ESP_LOGI(
                    TAG,
                    "Autofocus initialized"
                );}
            }
        }    

        /*
        * -----------------------------------------------------
        * VERIFY OV5640
        * -----------------------------------------------------
        */

        if (camera_log==1){ESP_LOGI(TAG,"Camera detected PID=0x%04X",sensor->id.PID);}


       if (sensor->id.PID != OV5640_PID)
        {
            ESP_LOGE(TAG, "Detected sensor is not OV5640");

            esp_camera_deinit();

            gpio_set_level(CAM_POWER_PIN, 1);

            photo_psram_address = NULL;
            photo_psram_size = 0;
            photo_capture_error = 1;

            vTaskDelay(pdMS_TO_TICKS(100));

            return ESP_FAIL;
        }



 

        if (camera_log==1){ESP_LOGI(TAG,"OV5640 initialized successfully");}
        //return ESP_OK;
    

    

    //static camera_fb_t *camera_take_photo(void);
    
    /*
 * =========================================================
 * AUTOFOCUS BEFORE CAPTURE
 * =========================================================
 */
        if(auto_focus==1)
        {
            if (autofocus_ready)
            {
                if (camera_log==1){ESP_LOGI(TAG, "Starting autofocus...");}

                esp_camera_af_status_t af_status;

                err = esp_camera_af_trigger(sensor);

                if (err != ESP_OK)
                {
                    if (camera_log==1){ESP_LOGW(
                        TAG,
                        "Autofocus trigger failed: %s",
                        esp_err_to_name(err)
                    );}
                }
                else
                {
                    err = esp_camera_af_wait(
                        sensor,
                        0,
                        &af_status
                    );

                    if (err == ESP_OK)
                    {
                       if (camera_log==1){ESP_LOGI(
                            TAG,
                            "Autofocus complete: focused=%d raw=0x%02X",
                            af_status.focused,
                            af_status.raw
                        );}
                    }
                    else
                    {
                        if (camera_log==1){ESP_LOGW(
                            TAG,
                            "Autofocus wait failed: %s",
                            esp_err_to_name(err)
                        );}
                    }
                }
            }
            else
            {
                if (camera_log==1){ESP_LOGW(
                    TAG,
                    "Autofocus unavailable - capturing without AF"
                );}
            }
        }    
    /*
    * =========================================================
    * TAKE ONE PHOTO
    * =========================================================
    */

        if (camera_log==1){ESP_LOGI(TAG,"Waiting for complete JPEG frame...");}

        /*
        * Standard driver function.
        *
        * Driver waits for a complete frame and
        * returns the framebuffer.
        */
        camera_fb_t *fb =esp_camera_fb_get();
    ///////////////////////////////////////GET PHOTO SIZE FROM CAMERA
        //sensor_t *sensor = esp_camera_sensor_get();
        
        int reg_4414 =sensor->get_reg(sensor, 0x4414, 0xFF);
        int reg_4415 =sensor->get_reg(sensor, 0x4415, 0xFF);
        int reg_4416 =sensor->get_reg(sensor, 0x4416, 0xFF);
        //int reg_4417 =sensor->get_reg(sensor, 0x4417, 0xFF);


        uint32_t jpeg_size =((uint32_t)reg_4414 << 16) |((uint32_t)reg_4415 << 8)  |((uint32_t)reg_4416);
        if (camera_log==1){printf("**CAMERA JPEG SIZE = %u bytes\n",(unsigned)jpeg_size);}
        
    ////////////////////////////////////////////////////////////////////
    ///////////////////////////////////image height and width
        int w_h = sensor->get_reg(sensor, 0x3808, 0xFF);
        int w_l = sensor->get_reg(sensor, 0x3809, 0xFF);
        int h_h = sensor->get_reg(sensor, 0x380A, 0xFF);
        int h_l = sensor->get_reg(sensor, 0x380B, 0xFF);

        uint16_t width =((w_h & 0x0F) << 8) |(w_l & 0xFF);
        uint16_t height =((h_h & 0x0F) << 8) |(h_l & 0xFF);

        if (camera_log==1){printf("**CAMERA FRAME SIZE = %u x %u\n",width,height);}
    /////////////////////////////////////////////////////////////////
    ///////////////////////////////////OVERFLOW CHECK(JFIFO OVERFLOW)
        int overflow_reg = sensor->get_reg(sensor,0x4417,0xFF);

        if (camera_log==1){printf("**CAMERA JFIFO OVERFLOW REG 0x4417 = 0x%02X\n",overflow_reg);}
        /*
        if (overflow_reg & 0x01)
        {
            printf("OV5640 JPEG FIFO OVERFLOW = YES\n");
        }
        else
        {
            printf("OV5640 JPEG FIFO OVERFLOW = NO\n");
        }
            */
    ////////////////////////////////////////////////////////////////////
        if (camera_log==1){ESP_LOGI(TAG,"JPEG QUALITY = %d",photo_quality);}
        

        if (fb == NULL)
        {
            ESP_LOGE(TAG, "Capture failed");

            photo_capture_error = 1;
            photo_psram_size = 0;

            

            gpio_set_level(CAM_POWER_PIN, 1);

            vTaskDelay(pdMS_TO_TICKS(100));

            return ESP_FAIL;
        }
  
        ESP_LOGI(TAG, "JPEG size in PSRAM: %u bytes",(unsigned)fb->len);

        if (camera_log==1){
            ESP_LOGI(TAG,"PHOTO CAPTURE COMPLETE");
            ESP_LOGI(TAG,"Resolution: %u x %u",(unsigned)fb->width,(unsigned)fb->height);
            ESP_LOGI(TAG,"Framebuffer address: %p",fb->buf);
        }

        saved_fb = fb;

        photo_psram_address = saved_fb->buf;
        photo_psram_size = saved_fb->len;

        photo_capture_error = 0;
        gpio_set_level(CAM_POWER_PIN, 1);



        /*
        * Useful JPEG verification.
        */
        if (fb->len >= 2)
        {
            if (camera_log==1){
                ESP_LOGI(TAG,"JPEG START = %02X %02X",fb->buf[0],fb->buf[1]);
                ESP_LOGI(TAG,"JPEG END   = %02X %02X",fb->buf[fb->len - 2],fb->buf[fb->len - 1]);
            }
        }

       
        /* Do NOT return framebuffer here.
        * WiFi still needs the JPEG in PSRAM.
        */

        photo_capture_error = 0;
        
                if (camera_log == 1)
        {
            ESP_LOGI(TAG,
                    "CAMERA POWER OFF - PHOTO KEPT IN PSRAM");
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        return ESP_OK;
    
    
}

 
/*
 * =========================================================
 * RETURN FRAMEBUFFER
 * =========================================================
 */

void camera_return_photo(camera_fb_t *fb)
{
    if (fb != NULL)
    {
        esp_camera_fb_return(fb);
    }
}



/*
 * =========================================================
 * SEND JPEG AS HEX THROUGH UART
 * =========================================================
 */

void camera_send_photo_uart(camera_fb_t *fb)
{
    if (fb == NULL)
    {
        return;
    }


    if (camera_log==1){printf("IMAGE_HEX_START:%u\n",(unsigned)fb->len);}


    for (size_t i = 0;i < fb->len;i++)
    {
        if (camera_log==1){printf("%02X",fb->buf[i]);}
    }


    if (camera_log==1){printf("\nIMAGE_HEX_END\n");}


    fflush(stdout);
}






    