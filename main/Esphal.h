#ifndef ESP_HAL_H
#define ESP_HAL_H

#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

#include <RadioLib.h>


class EspHal : public RadioLibHal
{
public:

    EspHal(
        int sckPin,
        int misoPin,
        int mosiPin
    )
    :
        RadioLibHal(
            GPIO_MODE_INPUT,
            GPIO_MODE_OUTPUT,
            0,
            1,
            GPIO_INTR_POSEDGE,
            GPIO_INTR_NEGEDGE
        ),

        spiSckPin(sckPin),
        spiMisoPin(misoPin),
        spiMosiPin(mosiPin),

        spiHost(SPI2_HOST),
        spiFrequency(2000000),

        spiDevice(nullptr),
        spiBusInitialized(false),
        spiDeviceAdded(false)
    {
    }


    void init() override
    {
        spiBegin();
    }


    void term() override
    {
        spiEnd();
    }


    void pinMode(
        uint32_t pin,
        uint32_t mode
    ) override
    {
        if(pin == RADIOLIB_NC)
        {
            return;
        }

        gpio_config_t config = {};

        config.pin_bit_mask = 1ULL << pin;
        config.mode = static_cast<gpio_mode_t>(mode);
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        config.intr_type = GPIO_INTR_DISABLE;

        esp_err_t result = gpio_config(&config);

        if(result != ESP_OK)
        {
            ESP_LOGE(
                "EspHal",
                "GPIO configuration failed: %s",
                esp_err_to_name(result)
            );
        }
    }


    void digitalWrite(
        uint32_t pin,
        uint32_t value
    ) override
    {
        if(pin == RADIOLIB_NC)
        {
            return;
        }

        gpio_set_level(
            static_cast<gpio_num_t>(pin),
            value
        );
    }


    uint32_t digitalRead(uint32_t pin) override
    {
        if(pin == RADIOLIB_NC)
        {
            return 0;
        }

        return gpio_get_level(
            static_cast<gpio_num_t>(pin)
        );
    }


   /* void attachInterrupt(
        uint32_t interruptNum,
        void (*interruptCb)(void),
        uint32_t mode
    ) override
    {
        if(
            interruptNum == RADIOLIB_NC ||
            interruptCb == nullptr
        )
        {
            return;
        }

        esp_err_t result = gpio_install_isr_service(
            ESP_INTR_FLAG_IRAM
        );

        if(
            result != ESP_OK &&
            result != ESP_ERR_INVALID_STATE
        )
        {
            ESP_LOGE(
                "EspHal",
                "GPIO ISR service installation failed: %s",
                esp_err_to_name(result)
            );

            return;
        }

        gpio_num_t pin =
            static_cast<gpio_num_t>(interruptNum);

        gpio_set_intr_type(
            pin,
            static_cast<gpio_int_type_t>(mode)
        );

        gpio_isr_handler_remove(pin);

        result = gpio_isr_handler_add(
            pin,
            gpioInterruptHandler,
            reinterpret_cast<void*>(interruptCb)
        );

        if(result != ESP_OK)
        {
            ESP_LOGE(
                "EspHal",
                "GPIO ISR handler installation failed: %s",
                esp_err_to_name(result)
            );
        }
    }*/

   void attachInterrupt(
    uint32_t interruptNum,
    void (*interruptCb)(void),
    uint32_t mode
) override
{
    if(
        interruptNum == RADIOLIB_NC ||
        interruptCb == nullptr
    )
    {
        return;
    }

    static bool isrServiceInstalled = false;

    if(!isrServiceInstalled)
    {
        esp_err_t result = gpio_install_isr_service(
            ESP_INTR_FLAG_IRAM
        );

        if(
            result == ESP_OK ||
            result == ESP_ERR_INVALID_STATE
        )
        {
            isrServiceInstalled = true;
        }
        else
        {
            /*ESP_LOGE(
                "EspHal",
                "GPIO ISR service installation failed: %s",
                esp_err_to_name(result)
            );*/

            return;
        }
    }

    gpio_num_t pin =
        static_cast<gpio_num_t>(interruptNum);

    gpio_set_direction(
        pin,
        GPIO_MODE_INPUT
    );

    gpio_set_intr_type(
        pin,
        static_cast<gpio_int_type_t>(mode)
    );

    // Remove an older handler if one exists.
    gpio_isr_handler_remove(pin);

    esp_err_t result = gpio_isr_handler_add(
        pin,
        gpioInterruptHandler,
        reinterpret_cast<void*>(interruptCb)
    );

    if(result != ESP_OK)
    {
        /*ESP_LOGE(
            "EspHal",
            "GPIO ISR handler installation failed: %s",
            esp_err_to_name(result)
        );*/
    }
    else
    {
        /*ESP_LOGI(
            "EspHal",
            "Interrupt attached to GPIO %lu, mode %lu",
            static_cast<unsigned long>(interruptNum),
            static_cast<unsigned long>(mode)
        );*/
    }
}


    void detachInterrupt(uint32_t interruptNum) override
    {
        if(interruptNum == RADIOLIB_NC)
        {
            return;
        }

        gpio_num_t pin =
            static_cast<gpio_num_t>(interruptNum);

        gpio_isr_handler_remove(pin);

        gpio_set_intr_type(
            pin,
            GPIO_INTR_DISABLE
        );
    }


    void delay(uint32_t ms) override
    {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }


    void delayMicroseconds(uint32_t us) override
    {
        esp_rom_delay_us(us);
    }


    uint32_t millis() override
    {
        return static_cast<uint32_t>(
            esp_timer_get_time() / 1000ULL
        );
    }


    uint32_t micros() override
    {
        return static_cast<uint32_t>(
            esp_timer_get_time()
        );
    }


    long pulseIn(
        uint32_t pin,
        uint32_t state,
        RadioLibTime_t timeout
    ) override
    {
        if(pin == RADIOLIB_NC)
        {
            return 0;
        }

        uint64_t startTime = esp_timer_get_time();

        while(
            digitalRead(pin) ==
            state
        )
        {
            if(
                esp_timer_get_time() - startTime >=
                static_cast<uint64_t>(timeout)
            )
            {
                return 0;
            }
        }

        while(
            digitalRead(pin) !=
            state
        )
        {
            if(
                esp_timer_get_time() - startTime >=
                static_cast<uint64_t>(timeout)
            )
            {
                return 0;
            }
        }

        uint64_t pulseStart = esp_timer_get_time();

        while(
            digitalRead(pin) ==
            state
        )
        {
            if(
                esp_timer_get_time() - startTime >=
                static_cast<uint64_t>(timeout)
            )
            {
                return 0;
            }
        }

        return static_cast<long>(
            esp_timer_get_time() - pulseStart
        );
    }


    void spiBegin() override
    {
        if(spiBusInitialized)
        {
            return;
        }

        spi_bus_config_t busConfig = {};

        busConfig.sclk_io_num = spiSckPin;
        busConfig.miso_io_num = spiMisoPin;
        busConfig.mosi_io_num = spiMosiPin;

        busConfig.quadwp_io_num = -1;
        busConfig.quadhd_io_num = -1;

        busConfig.max_transfer_sz = 256;

        esp_err_t result = spi_bus_initialize(
            spiHost,
            &busConfig,
            SPI_DMA_CH_AUTO
        );

        if(result == ESP_OK)
        {
            spiBusInitialized = true;
        }
        else if(result == ESP_ERR_INVALID_STATE)
        {
            spiBusInitialized = true;
        }
        else
        {
            ESP_LOGE(
                "EspHal",
                "SPI bus initialization failed: %s",
                esp_err_to_name(result)
            );
        }
    }


    void spiBeginTransaction() override
    {
        if(!spiDeviceAdded)
        {
            spi_device_interface_config_t deviceConfig = {};

            deviceConfig.clock_speed_hz = spiFrequency;
            deviceConfig.mode = 0;
            deviceConfig.spics_io_num = -1;
            deviceConfig.queue_size = 1;

            esp_err_t result = spi_bus_add_device(
                spiHost,
                &deviceConfig,
                &spiDevice
            );

            if(result != ESP_OK)
            {
                ESP_LOGE(
                    "EspHal",
                    "SPI device creation failed: %s",
                    esp_err_to_name(result)
                );

                return;
            }

            spiDeviceAdded = true;
        }

        if(spiDevice != nullptr)
        {
            esp_err_t result =
                spi_device_acquire_bus(
                    spiDevice,
                    portMAX_DELAY
                );

            if(result != ESP_OK)
            {
                ESP_LOGE(
                    "EspHal",
                    "SPI bus acquisition failed: %s",
                    esp_err_to_name(result)
                );
            }
        }
    }


    void spiTransfer(
        uint8_t* out,
        size_t len,
        uint8_t* in
    ) override
    {
        if(
            spiDevice == nullptr ||
            len == 0
        )
        {
            return;
        }

        spi_transaction_t transaction = {};

        transaction.length = len * 8;
        transaction.tx_buffer = out;
        transaction.rx_buffer = in;

        esp_err_t result =
            spi_device_polling_transmit(
                spiDevice,
                &transaction
            );

        if(result != ESP_OK)
        {
            ESP_LOGE(
                "EspHal",
                "SPI transfer failed: %s",
                esp_err_to_name(result)
            );
        }
    }


    void spiEndTransaction() override
    {
        if(spiDevice != nullptr)
        {
            spi_device_release_bus(spiDevice);
        }
    }


    void spiEnd() override
    {
        if(
            spiDeviceAdded &&
            spiDevice != nullptr
        )
        {
            esp_err_t result =
                spi_bus_remove_device(spiDevice);

            if(result == ESP_OK)
            {
                spiDevice = nullptr;
                spiDeviceAdded = false;
            }
            else
            {
                ESP_LOGE(
                    "EspHal",
                    "SPI device removal failed: %s",
                    esp_err_to_name(result)
                );
            }
        }

        if(
            spiBusInitialized &&
            !spiDeviceAdded
        )
        {
            esp_err_t result =
                spi_bus_free(spiHost);

            if(result == ESP_OK)
            {
                spiBusInitialized = false;
            }
            else
            {
                ESP_LOGE(
                    "EspHal",
                    "SPI bus release failed: %s",
                    esp_err_to_name(result)
                );
            }
        }
    }


private:

    static void IRAM_ATTR gpioInterruptHandler(
        void* argument
    )
    {
        auto callback =
            reinterpret_cast<void (*)(void)>(
                argument
            );

        if(callback != nullptr)
        {
            callback();
        }
    }


    int spiSckPin;
    int spiMisoPin;
    int spiMosiPin;

    spi_host_device_t spiHost;
    uint32_t spiFrequency;

    spi_device_handle_t spiDevice;

    bool spiBusInitialized;
    bool spiDeviceAdded;
};

#endif