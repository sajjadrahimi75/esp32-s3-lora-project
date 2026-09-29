#include "camera.h"
#include "driver/uart.h"
#include "esp_camera.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



static const char *TAG = "CAMERA";


#define CAM_POWER_PIN   3

#define CAM_PIN_PWDN    16
#define CAM_PIN_RESET   -1

#define CAM_PIN_XCLK    48

#define CAM_PIN_SIOD    6
#define CAM_PIN_SIOC    5

#define CAM_PIN_VSYNC   15
#define CAM_PIN_HREF    19
#define CAM_PIN_PCLK    42

#define CAM_PIN_D0      40   // OV5640 D2
#define CAM_PIN_D1      38   // OV5640 D3
#define CAM_PIN_D2      37   // OV5640 D4
#define CAM_PIN_D3      39   // OV5640 D5
#define CAM_PIN_D4      41   // OV5640 D6
#define CAM_PIN_D5      36   // OV5640 D7
#define CAM_PIN_D6      35   // OV5640 D8
#define CAM_PIN_D7      20   // OV5640 D9

esp_err_t camera_init_ov5640(void)
{
    /*
     * Camera power ON.
     *
     * Q3 is a P-channel high-side MOSFET,
     * so CAMOn LOW should enable VCam.
     */
    gpio_reset_pin(CAM_POWER_PIN);
    gpio_set_direction(CAM_POWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(CAM_POWER_PIN, 0);

    vTaskDelay(pdMS_TO_TICKS(100));
    
 
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

        .xclk_freq_hz = 10000000,//20000000,

        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,//PIXFORMAT_RGB565

        /*
         * Start small while debugging.
         */
        .frame_size = FRAMESIZE_QSXGA,//FRAMESIZE_5MP,FRAMESIZE_UXGA,FRAMESIZE_QVGA,FRAMESIZE_VGA,

        .jpeg_quality = 5,//12

        .fb_count = 1,

        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,

        .fb_location = CAMERA_FB_IN_PSRAM,//CAMERA_FB_IN_DRAM,

        
    };


    ESP_LOGI(TAG, "Initializing OV5640...");

    esp_err_t err =
        esp_camera_init(&config);

    if(err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Camera init failed: 0x%x",
                 err);

        return err;
    }


    sensor_t *sensor =
        esp_camera_sensor_get();

        if(sensor == NULL)
    {
        ESP_LOGE(TAG,
                 "Camera sensor not available");

        return ESP_FAIL;
    }

    ///////////////////////////////////camera parameter//////////////////////
    // Camera image enhancement settings

    sensor->set_sharpness(sensor, 2);    // Maximum digital sharpening(-3,+3)
    sensor->set_contrast(sensor, 3);     // Increased contrast(-3,+3)
    sensor->set_brightness(sensor, 2);   // Normal brightness(-3,+3)
    sensor->set_saturation(sensor, 0);   // Normal saturation(-4,+4)

    sensor->set_whitebal(sensor, 1);     // Enable automatic white balance
    sensor->set_exposure_ctrl(sensor, 1); // Enable automatic exposure
    sensor->set_gain_ctrl(sensor, 1);    // Enable automatic gain

    sensor->set_colorbar(sensor, 0);     // Disable test pattern
    /////////////////////////////////////////////////////////////////////////


    ESP_LOGI(TAG, "REG 3008 = 0x%02X", sensor->get_reg(sensor, 0x3008, 0xFF));
    ESP_LOGI(TAG, "REG 3007 = 0x%02X", sensor->get_reg(sensor, 0x3007, 0xFF));
    ESP_LOGI(TAG, "REG 300E = 0x%02X", sensor->get_reg(sensor, 0x300E, 0xFF));
    ESP_LOGI(TAG, "REG 3017 = 0x%02X", sensor->get_reg(sensor, 0x3017, 0xFF));
    ESP_LOGI(TAG, "REG 3018 = 0x%02X", sensor->get_reg(sensor, 0x3018, 0xFF));   

    


    ESP_LOGI(TAG, "Enabling OV5640 DVP outputs");

    sensor->set_reg(sensor, 0x3017, 0xFF, 0xFF);
    sensor->set_reg(sensor, 0x3018, 0xFF, 0xFF);

        // Set OV5640 output drive strength to 4x
    sensor->set_reg(sensor, 0x302C, 0xC0, 0xC0);

    // Verify the register
    ESP_LOGI(TAG, "REG 302C = 0x%02X",
            sensor->get_reg(sensor, 0x302C, 0xFF));

    /////////////////////////////////////////////
    if (sensor == NULL)
{
    ESP_LOGE(TAG, "Camera sensor not available");
    return ESP_FAIL;
}

/////////////////////////////////////////////////////
    
    vTaskDelay(pdMS_TO_TICKS(300));

    // discard startup frames
  /* for (int i = 0; i < 2; i++)
    {
        camera_fb_t *fb = esp_camera_fb_get();

        if (fb != NULL)
        {
            esp_camera_fb_return(fb);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }*/



    ESP_LOGI(TAG, "REG 3017 = 0x%02X",
            sensor->get_reg(sensor, 0x3017, 0xFF));

    ESP_LOGI(TAG, "REG 3018 = 0x%02X",
            sensor->get_reg(sensor, 0x3018, 0xFF));


    ESP_LOGI(TAG,
             "Camera detected PID=0x%04X",
             sensor->id.PID);

    ESP_LOGI(TAG,
             "OV5640 initialized successfully");

    return ESP_OK;
}





camera_fb_t *camera_take_photo(void)
{
    camera_fb_t *fb =
        esp_camera_fb_get();

    if(fb == NULL)
    {
        ESP_LOGE(TAG,
                 "Capture failed");

        return NULL;
    }

    //ESP_LOGI(TAG,"JPEG captured: %u bytes, %ux%u",(unsigned)fb->len,fb->width,fb->height);
    ESP_LOGI(TAG,"Frame captured: %u bytes, %ux%u",(unsigned)fb->len,fb->width,fb->height);
    gpio_set_level(CAM_POWER_PIN, 1);
    ESP_LOGI(TAG, "CAMERA POWER OFF - JPEG stored in PSRAM");
    return fb;
}


void camera_return_photo(camera_fb_t *fb)
{
    if(fb != NULL)
    {
        esp_camera_fb_return(fb);
    }


    
}

    ///////////////////////////////////sending photo
/*void camera_send_photo_uart(camera_fb_t *fb)
{
    if (fb == NULL) {
        return;
    }

    printf("\nIMAGE_START:%u\n", (unsigned)fb->len);
    fflush(stdout);

    uart_write_bytes(UART_NUM_0,
                     (const char *)fb->buf,
                     fb->len);

    uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(5000));

    printf("\nIMAGE_END\n");
    fflush(stdout);
}*/



void camera_send_photo_uart(camera_fb_t *fb)
{
    if (fb == NULL) {
        return;
    }

    printf("IMAGE_HEX_START:%u\n", (unsigned)fb->len);

        for (size_t i = 0; i < fb->len; i++)
    {
        printf("%02X", fb->buf[i]);
    }

    printf("\nIMAGE_HEX_END\n");
    fflush(stdout);
 
}