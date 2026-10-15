#include "camera.h"

#include <stdio.h>
#include <stdint.h>

#include "esp_camera.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "CAMERA";


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



/*
 * =========================================================
 * CAMERA INITIALIZATION
 * =========================================================
 */

esp_err_t camera_init_ov5640(void)
{
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

    gpio_set_direction(
        CAM_POWER_PIN,
        GPIO_MODE_OUTPUT
    );

    gpio_set_level(
        CAM_POWER_PIN,
        0
    );


    /*
     * Give camera power some time to stabilize.
     */
    vTaskDelay(
        pdMS_TO_TICKS(100)
    );


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
        .xclk_freq_hz = 6000000,


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


    ESP_LOGI(TAG,"Initializing OV5640...");


    /*
     * -----------------------------------------------------
     * INITIALIZE ORIGINAL ESP32-CAMERA DRIVER
     * -----------------------------------------------------
     */

    esp_err_t err =esp_camera_init(&config);


    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,"Camera init failed: 0x%x",err);
        return err;
    }



    /*
     * -----------------------------------------------------
     * GET CAMERA SENSOR
     * -----------------------------------------------------
     */

    sensor_t *sensor =esp_camera_sensor_get();


    if (sensor == NULL)
    {
        ESP_LOGE(TAG,"Camera sensor not available");

        return ESP_FAIL;
    }



    /*
     * -----------------------------------------------------
     * VERIFY OV5640
     * -----------------------------------------------------
     */

    ESP_LOGI(TAG,"Camera detected PID=0x%04X",sensor->id.PID);


    if (sensor->id.PID != OV5640_PID)
    {
        ESP_LOGE(TAG,"Detected sensor is not OV5640");

        esp_camera_deinit();

        return ESP_FAIL;
    }



    /*
     * -----------------------------------------------------
     * BASIC IMAGE SETTINGS
     * -----------------------------------------------------
     */

    sensor->set_sharpness(sensor,0);

    sensor->set_contrast(sensor,0);

    sensor->set_brightness(sensor,0);

    sensor->set_saturation(sensor,0);


    sensor->set_whitebal(sensor,1);

    sensor->set_exposure_ctrl(sensor,1);

    sensor->set_gain_ctrl(sensor,1);

    sensor->set_colorbar(sensor,0);


    ESP_LOGI(TAG,"OV5640 initialized successfully");
    return ESP_OK;
}



/*
 * =========================================================
 * TAKE ONE PHOTO
 * =========================================================
 */

camera_fb_t *camera_take_photo(void)
{
    ESP_LOGI(TAG,"Waiting for complete JPEG frame...");
    /*
     * Standard driver function.
     *
     * Driver waits for a complete frame and
     * returns the framebuffer.
     */
    camera_fb_t *fb =esp_camera_fb_get();
///////////////////////////////////////GET PHOTO SIZE FROM CAMERA
    sensor_t *sensor = esp_camera_sensor_get();
    
    int reg_4414 =sensor->get_reg(sensor, 0x4414, 0xFF);
    int reg_4415 =sensor->get_reg(sensor, 0x4415, 0xFF);
    int reg_4416 =sensor->get_reg(sensor, 0x4416, 0xFF);
    int reg_4417 =sensor->get_reg(sensor, 0x4417, 0xFF);


    uint32_t jpeg_size =((uint32_t)reg_4414 << 16) |((uint32_t)reg_4415 << 8)  |((uint32_t)reg_4416);
    printf("**CAMERA JPEG SIZE = %u bytes\n",(unsigned)jpeg_size);
    
////////////////////////////////////////////////////////////////////
///////////////////////////////////image height and width
    int w_h = sensor->get_reg(sensor, 0x3808, 0xFF);
    int w_l = sensor->get_reg(sensor, 0x3809, 0xFF);
    int h_h = sensor->get_reg(sensor, 0x380A, 0xFF);
    int h_l = sensor->get_reg(sensor, 0x380B, 0xFF);

    uint16_t width =((w_h & 0x0F) << 8) |(w_l & 0xFF);
    uint16_t height =((h_h & 0x0F) << 8) |(h_l & 0xFF);

    printf("**CAMERA FRAME SIZE = %u x %u\n",width,height);
/////////////////////////////////////////////////////////////////
///////////////////////////////////OVERFLOW CHECK(JFIFO OVERFLOW)
    int overflow_reg = sensor->get_reg(sensor,0x4417,0xFF);

    printf("**CAMERA JFIFO OVERFLOW REG 0x4417 = 0x%02X\n",overflow_reg);
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
    ESP_LOGI(TAG,"JPEG QUALITY = %d",photo_quality);
    

    if (fb == NULL)
    {
        ESP_LOGE( TAG,"Capture failed");
        return NULL;
    }

    ESP_LOGI(TAG,"PHOTO CAPTURE COMPLETE");
    ESP_LOGI(TAG, "JPEG size in SPRAM: %u bytes",(unsigned)fb->len);
    //ESP_LOGI(TAG,"Resolution: %u x %u",(unsigned)fb->width,(unsigned)fb->height);

    ESP_LOGI(TAG,"Framebuffer address: %p",fb->buf);


    /*
     * Useful JPEG verification.
     */
    if (fb->len >= 2)
    {
        ESP_LOGI(TAG,"JPEG START = %02X %02X",fb->buf[0],fb->buf[1]);
        ESP_LOGI(TAG,"JPEG END   = %02X %02X",fb->buf[fb->len - 2],fb->buf[fb->len - 1]);
    }


    return fb;
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


    printf("IMAGE_HEX_START:%u\n",(unsigned)fb->len);


    for (size_t i = 0;i < fb->len;i++)
    {
        printf("%02X",fb->buf[i]);
    }


    printf("\nIMAGE_HEX_END\n");


    fflush(stdout);
}



/*
 * =========================================================
 * POWER OFF CAMERA
 * =========================================================
 */

void camera_power_off(void)
{
    /*
     * First stop and free the standard camera driver.
     */
    esp_camera_deinit();


    /*
     * Then remove power from OV5640.
     */
    gpio_set_level(CAM_POWER_PIN,1);


    ESP_LOGI(TAG,"CAMERA POWER OFF");
}