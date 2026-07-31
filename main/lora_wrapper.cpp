#include <RadioLib.h>

#include "EspHal.h"

#include "driver/gpio.h"
#include "esp_log.h"


// SPI pins:
// SCK  = GPIO14
// MISO = GPIO11
// MOSI = GPIO13
EspHal* hal = new EspHal(
    14,
    11,
    13
);


// SX1262 pins:
// NSS   = GPIO10
// DIO1  = GPIO46
// RESET = GPIO9
// BUSY  = GPIO21
SX1262 radio = new Module(
    hal,
    10,
    46,
    9,
    21
);


static const char* TAG = "LoRa";


extern "C" void app_main(void)
{
    // GPIO8 controls power to the LoRa module.
 
ESP_LOGI(TAG, "STEP 1: Turning ON LoRa module...");

gpio_reset_pin(GPIO_NUM_47);
gpio_set_direction(GPIO_NUM_47,GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_47,1); //TURN ON LED


gpio_reset_pin(GPIO_NUM_21);
gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT);

gpio_reset_pin(GPIO_NUM_8);
gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_8, 0);      // LOW = LoRa ON

//reset
gpio_set_direction(GPIO_NUM_9, GPIO_MODE_INPUT_OUTPUT);
gpio_set_level(GPIO_NUM_9, 0);
hal->delay(10);
ESP_LOGI(TAG, "RESET LORA (GPIO LEVE IS:) = %d", gpio_get_level(GPIO_NUM_9));

gpio_set_level(GPIO_NUM_9, 1);
ESP_LOGI(TAG, "RELEASE RESET (GPIO LEVE IS:) = %d", gpio_get_level(GPIO_NUM_9));

bool busy_high = false;

for(int i = 0; i < 1000; i++)      // wait up to 1000 ms
{

    if(gpio_get_level(GPIO_NUM_21) == 1)
    {
        ESP_LOGI(TAG, "PASS: BUSY became HIGH after %d ms", i * 10);
        busy_high = true;

        if(gpio_get_level(GPIO_NUM_21)==0)
        {
            ESP_LOGI(TAG, "STEP 1 PASSED --BUSY=0");
            break;
        }
        
    }

    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(GPIO_NUM_47,0);
}

if(!busy_high)
{
    ESP_LOGE(TAG, "FAIL: BUSY never became HIGH after power ON");

    while(true)
    {
        gpio_set_level(GPIO_NUM_47,0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_NUM_47,1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


//end gpio8 test




gpio_set_direction(GPIO_NUM_46, GPIO_MODE_INPUT);

ESP_LOGI(TAG, "DIO1 level = %d", gpio_get_level(GPIO_NUM_46));

ESP_LOGI(TAG, "LoRa power enabled");
    // Wait for the module power to become stable.
    hal->delay(200);

    ESP_LOGI(TAG, "Initializing SX1262...");


    //RESET LORA BEFORE START




//TEST SPI BUSY
gpio_reset_pin(GPIO_NUM_10);      // NSS
gpio_set_direction(GPIO_NUM_10, GPIO_MODE_INPUT_OUTPUT);
gpio_set_level(GPIO_NUM_10, 1);   // Idle state

while(gpio_get_level(GPIO_NUM_21) == 1)
{
    vTaskDelay(pdMS_TO_TICKS(1));
}

hal->spiBegin();
hal->spiBeginTransaction();

gpio_set_level(GPIO_NUM_10, 0);   // CS LOW
ESP_LOGI(TAG, "NSS = %d", gpio_get_level(GPIO_NUM_10));

uint8_t tx[2] = {0xC0, 0x00};
uint8_t rx[2] = {0};

hal->spiTransfer(tx, 2, rx);

gpio_set_level(GPIO_NUM_10, 1);   // CS HIGH
ESP_LOGI(TAG, "NSS = %d", gpio_get_level(GPIO_NUM_10));

hal->spiEndTransaction();

ESP_LOGI(TAG, "GetStatus RX = %02X %02X", rx[0], rx[1]);


uint8_t status = rx[1];

uint8_t chip_mode = (status >> 4) & 0x07;
uint8_t command_status = (status >> 1) & 0x07;

ESP_LOGI(TAG, "Status byte = 0x%02X", status);
ESP_LOGI(TAG, "Chip mode = %u", chip_mode);
ESP_LOGI(TAG, "Command status = %u", command_status);


int16_t state = radio.begin(868.0, 125.0, 9, 7, 0x12, 14, 8, 0.0);

    

    if(state != RADIOLIB_ERR_NONE)
    {
        ESP_LOGE(
            TAG,
            "SX1262 initialization failed, code: %d",
            state
        );

        while(true)
        {
            hal->delay(1000);
        }
    }

    ESP_LOGI(
        TAG,
        "SX1262 initialization successful"
    );

    while(true)
    {
        ESP_LOGI(
            TAG,
            "Transmitting: Hello World!"
        );

        state = radio.transmit("Hello World!");

        if(state == RADIOLIB_ERR_NONE)
        {
            ESP_LOGI(
                TAG,
                "Transmission successful"
            );
        }
        else
        {
            ESP_LOGE(
                TAG,
                "Transmission failed, code: %d",
                state
            );
        }

        hal->delay(3000);
    }
}