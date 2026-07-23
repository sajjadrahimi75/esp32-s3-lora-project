#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

#include "RadioLib.h"
#include "lora_wrapper.h"

/*
 * ESP-IDF hardware abstraction layer for RadioLib.
 *
 * It is placed directly inside lora_wrapper.cpp,
 * so EspHal.cpp and EspHal.h are not needed.
 */
class EspHal : public RadioLibHal
{
public:

    EspHal(
        int sck_pin,
        int miso_pin,
        int mosi_pin,
        spi_host_device_t selected_spi_host,
        uint32_t selected_spi_frequency
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

        spi_sck_pin(sck_pin),
        spi_miso_pin(miso_pin),
        spi_mosi_pin(mosi_pin),
        spi_host(selected_spi_host),
        spi_frequency(selected_spi_frequency),
        spi_device(nullptr),
        spi_bus_initialized(false),
        spi_device_added(false),
        hal_initialized(false)
    {
    }

    ~EspHal() override
    {
        term();
    }

    /*
     * Initialize SPI and the GPIO interrupt service.
     */
    void init() override
    {
        if (hal_initialized)
        {
            return;
        }

        spiBegin();

        esp_err_t result =
            gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

        /*
         * ESP_ERR_INVALID_STATE means that another
         * part of the program already installed it.
         */
        if (
            result != ESP_OK &&
            result != ESP_ERR_INVALID_STATE
        )
        {
            ESP_LOGE(
                "EspHal",
                "GPIO ISR installation failed: %s",
                esp_err_to_name(result)
            );
        }

        hal_initialized = true;
    }

    /*
     * Stop the HAL.
     */
    void term() override
    {
        if (!hal_initialized)
        {
            return;
        }

        spiEnd();

        hal_initialized = false;
    }

    /*
     * Configure a pin as an input or output.
     */
    void pinMode(
        uint32_t pin,
        uint32_t mode
    ) override
    {
        if (pin == RADIOLIB_NC)
        {
            return;
        }

        gpio_config_t configuration = {};

        configuration.pin_bit_mask =
            (1ULL << pin);

        if (mode == GPIO_MODE_OUTPUT)
        {
            configuration.mode =
                GPIO_MODE_OUTPUT;
        }
        else
        {
            configuration.mode =
                GPIO_MODE_INPUT;
        }

        configuration.pull_up_en =
            GPIO_PULLUP_DISABLE;

        configuration.pull_down_en =
            GPIO_PULLDOWN_DISABLE;

        configuration.intr_type =
            GPIO_INTR_DISABLE;

        gpio_config(&configuration);
    }

    /*
     * Write HIGH or LOW to a GPIO.
     */
    void digitalWrite(
        uint32_t pin,
        uint32_t value
    ) override
    {
        if (pin == RADIOLIB_NC)
        {
            return;
        }

        gpio_set_level(
            (gpio_num_t)pin,
            value
        );
    }

    /*
     * Read a GPIO.
     */
    uint32_t digitalRead(
        uint32_t pin
    ) override
    {
        if (pin == RADIOLIB_NC)
        {
            return 0;
        }

        return gpio_get_level(
            (gpio_num_t)pin
        );
    }

    /*
     * Attach a GPIO interrupt.
     */
    void attachInterrupt(
        uint32_t interrupt_number,
        void (*interrupt_callback)(void),
        uint32_t mode
    ) override
    {
        if (interrupt_number == RADIOLIB_NC)
        {
            return;
        }

        gpio_set_intr_type(
            (gpio_num_t)interrupt_number,
            (gpio_int_type_t)mode
        );

        gpio_isr_handler_add(
            (gpio_num_t)interrupt_number,
            (gpio_isr_t)interrupt_callback,
            nullptr
        );
    }

    /*
     * Remove a GPIO interrupt.
     */
    void detachInterrupt(
        uint32_t interrupt_number
    ) override
    {
        if (interrupt_number == RADIOLIB_NC)
        {
            return;
        }

        gpio_isr_handler_remove(
            (gpio_num_t)interrupt_number
        );
    }

    /*
     * Delay in milliseconds.
     */
    void delay(
        RadioLibTime_t milliseconds
    ) override
    {
        if (milliseconds == 0)
        {
            return;
        }

        vTaskDelay(
            pdMS_TO_TICKS(milliseconds)
        );
    }

    /*
     * Delay in microseconds.
     */
    void delayMicroseconds(
        RadioLibTime_t microseconds
    ) override
    {
        esp_rom_delay_us(
            (uint32_t)microseconds
        );
    }

    /*
     * Time since startup in milliseconds.
     */
    RadioLibTime_t millis() override
    {
        return (RadioLibTime_t)(
            esp_timer_get_time() / 1000ULL
        );
    }

    /*
     * Time since startup in microseconds.
     */
    RadioLibTime_t micros() override
    {
        return (RadioLibTime_t)(
            esp_timer_get_time()
        );
    }

    /*
     * Measure the length of a GPIO pulse.
     */
    long pulseIn(
        uint32_t pin,
        uint32_t state,
        RadioLibTime_t timeout
    ) override
    {
        if (pin == RADIOLIB_NC)
        {
            return 0;
        }

        RadioLibTime_t start_time = micros();

        while (digitalRead(pin) != state)
        {
            if ((micros() - start_time) >= timeout)
            {
                return 0;
            }
        }

        RadioLibTime_t pulse_start = micros();

        while (digitalRead(pin) == state)
        {
            if ((micros() - pulse_start) >= timeout)
            {
                return 0;
            }
        }

        return (long)(
            micros() - pulse_start
        );
    }

    /*
     * Initialize the ESP32-S3 SPI peripheral.
     */
    void spiBegin() override
    {
        if (spi_device_added)
        {
            return;
        }

        spi_bus_config_t bus_configuration = {};

        bus_configuration.mosi_io_num =
            spi_mosi_pin;

        bus_configuration.miso_io_num =
            spi_miso_pin;

        bus_configuration.sclk_io_num =
            spi_sck_pin;

        bus_configuration.quadwp_io_num =
            -1;

        bus_configuration.quadhd_io_num =
            -1;

        bus_configuration.max_transfer_sz =
            512;

        esp_err_t result =
            spi_bus_initialize(
                spi_host,
                &bus_configuration,
                SPI_DMA_CH_AUTO
            );

        if (result == ESP_OK)
        {
            spi_bus_initialized = true;
        }
        else if (result == ESP_ERR_INVALID_STATE)
        {
            /*
             * The SPI bus was already initialized.
             */
            spi_bus_initialized = false;
        }
        else
        {
            ESP_LOGE(
                "EspHal",
                "SPI bus initialization failed: %s",
                esp_err_to_name(result)
            );

            return;
        }

        spi_device_interface_config_t device_configuration = {};

        /*
         * SX1262 uses SPI mode 0.
         */
        device_configuration.mode = 0;

        device_configuration.clock_speed_hz =
            spi_frequency;

        /*
         * RadioLib manually controls NSS.
         */
        device_configuration.spics_io_num =
            -1;

        device_configuration.queue_size =
            1;

        result =
            spi_bus_add_device(
                spi_host,
                &device_configuration,
                &spi_device
            );

        if (result != ESP_OK)
        {
            ESP_LOGE(
                "EspHal",
                "SPI device creation failed: %s",
                esp_err_to_name(result)
            );

            return;
        }

        spi_device_added = true;
    }

    /*
     * Reserve the SPI bus.
     */
    void spiBeginTransaction() override
    {
        if (spi_device == nullptr)
        {
            return;
        }

        spi_device_acquire_bus(
            spi_device,
            portMAX_DELAY
        );
    }

    /*
     * Send and receive SPI data.
     */
    void spiTransfer(
    uint8_t *output,
    size_t length,
    uint8_t *input
) override
{
    if (
        spi_device == nullptr ||
        length == 0
    )
    {
        printf("SPI transfer skipped: invalid device\r\n");
        return;
    }

    spi_transaction_t transaction = {};

    transaction.length = length * 8;
    transaction.tx_buffer = output;
    transaction.rx_buffer = input;

    esp_err_t result =
        spi_device_polling_transmit(
            spi_device,
            &transaction
        );

    if (result != ESP_OK)
    {
        printf(
            "SPI transfer error: %s\r\n",
            esp_err_to_name(result)
        );

        return;
    }

    static int diagnostic_count = 0;

    if (diagnostic_count < 20)
    {
        printf("SPI TX:");

        for (size_t i = 0; i < length; i++)
        {
            printf(" %02X", output[i]);
        }

        printf("\r\nSPI RX:");

        if (input != nullptr)
        {
            for (size_t i = 0; i < length; i++)
            {
                printf(" %02X", input[i]);
            }
        }
        else
        {
            printf(" NULL");
        }

        printf("\r\n");

        diagnostic_count++;
    }
}

    /*
     * Release the SPI bus.
     */
    void spiEndTransaction() override
    {
        if (spi_device == nullptr)
        {
            return;
        }

        spi_device_release_bus(
            spi_device
        );
    }

    /*
     * Remove the SPI device.
     */
    void spiEnd() override
    {
        if (
            spi_device_added &&
            spi_device != nullptr
        )
        {
            spi_bus_remove_device(
                spi_device
            );

            spi_device = nullptr;
            spi_device_added = false;
        }

        if (spi_bus_initialized)
        {
            spi_bus_free(
                spi_host
            );

            spi_bus_initialized = false;
        }
    }

    /*
     * Allow other FreeRTOS tasks to execute.
     */
    void yield() override
    {
        taskYIELD();
    }

    /*
     * Configure GPIO pull-up or pull-down.
     */
    void pullUpDown(
        uint32_t pin,
        bool enable,
        bool up
    ) override
    {
        if (pin == RADIOLIB_NC)
        {
            return;
        }

        if (!enable)
        {
            gpio_set_pull_mode(
                (gpio_num_t)pin,
                GPIO_FLOATING
            );

            return;
        }

        if (up)
        {
            gpio_set_pull_mode(
                (gpio_num_t)pin,
                GPIO_PULLUP_ONLY
            );
        }
        else
        {
            gpio_set_pull_mode(
                (gpio_num_t)pin,
                GPIO_PULLDOWN_ONLY
            );
        }
    }

    /*
     * Tone generation is not needed for SX1262.
     */
    void tone(
        uint32_t pin,
        unsigned int frequency,
        RadioLibTime_t duration = 0
    ) override
    {
        (void)pin;
        (void)frequency;
        (void)duration;
    }

    /*
     * Tone generation is not needed for SX1262.
     */
    void noTone(
        uint32_t pin
    ) override
    {
        (void)pin;
    }

private:

    int spi_sck_pin;
    int spi_miso_pin;
    int spi_mosi_pin;

    spi_host_device_t spi_host;
    uint32_t spi_frequency;

    spi_device_handle_t spi_device;

    bool spi_bus_initialized;
    bool spi_device_added;
    bool hal_initialized;
};

/*
 * Your board connections from the schematic:
 *
 * GPIO5  = LoRa power enable
 * GPIO8  = DIO1
 * GPIO9  = reset
 * GPIO10 = NSS / chip select
 * GPIO11 = MISO
 * GPIO13 = MOSI
 * GPIO14 = SCK
 * GPIO21 = BUSY
 */
static const int LORA_POWER_PIN = 8;

static const int LORA_CS_PIN   = 10;
static const int LORA_DIO1_PIN = 46;
static const int LORA_RST_PIN  = 9;
static const int LORA_BUSY_PIN = 21;

static const int LORA_SCK_PIN  = 14;
static const int LORA_MISO_PIN = 11;
static const int LORA_MOSI_PIN = 13;

/*
 * Replace these values with the values
 * generated by The Things Stack.
 */
static const uint64_t JOIN_EUI =
    0x0000000000000000ULL;

static const uint64_t DEV_EUI =
    0x70B3D57ED00787BFULL;

/*
 * Replace these bytes with your AppKey.
 */
static uint8_t APP_KEY[16] =
{
    0x9B, 0xB0, 0x36, 0x7D,
    0x49, 0xCA, 0xF7, 0x96,
    0x5C, 0xF5, 0x73, 0xE6,
    0xD9, 0x21, 0xFE, 0x6A
};

/*
 * For LoRaWAN 1.0.x, use the same key
 * value here as APP_KEY.
 */
static uint8_t NWK_KEY[16] =
{
    0x9B, 0xB0, 0x36, 0x7D,
    0x49, 0xCA, 0xF7, 0x96,
    0x5C, 0xF5, 0x73, 0xE6,
    0xD9, 0x21, 0xFE, 0x6A
};

/*
 * Create the HAL using the actual SPI pins.
 */
static EspHal hal(
    LORA_SCK_PIN,
    LORA_MISO_PIN,
    LORA_MOSI_PIN,
    SPI2_HOST,
    500000
);

/*
 * RadioLib Module constructor:
 *
 * HAL, NSS, DIO1, RESET, BUSY
 */
static Module radio_module(
    &hal,
    LORA_CS_PIN,
    LORA_DIO1_PIN,
    LORA_RST_PIN,
    LORA_BUSY_PIN
);

/*
 * Create the SX1262 object only once.
 */
static SX1262 radio(
    &radio_module
);


/*
 * Europe 863–870 MHz LoRaWAN region.
 */
static LoRaWANNode node(
    &radio,
    &EU868
);

/*
 * Shows whether the device joined the network.
 */
static bool lora_joined = false;

extern "C" int lora_init(void)
{
    /*
     * Turn on V_Lora.
     * According to your schematic, GPIO8 LOW enables Q1.
     */
    gpio_reset_pin((gpio_num_t)LORA_POWER_PIN);

    gpio_set_direction(
        (gpio_num_t)LORA_POWER_PIN,
        GPIO_MODE_OUTPUT
    );

    gpio_set_level(
        (gpio_num_t)LORA_POWER_PIN,
        0
    );

    vTaskDelay(pdMS_TO_TICKS(500));

    printf(
        "Lora power control: GPIO8=%d\r\n",
        gpio_get_level((gpio_num_t)LORA_POWER_PIN)
    );

    printf("Starting SX1262 SPI test...\r\n");

    /*
     * Your schematic uses a DIO3-controlled 3.3 V TCXO.
     */
    int16_t state = radio.begin(
        868.0,
        125.0,
        9,
        7,
        0x12,
        14,
        8,
        0.0,
        false
    );

    printf(
        "radio.begin result = %d\r\n",
        state
    );

    if (state != RADIOLIB_ERR_NONE)
    {
        printf("SX1262 initialization failed\r\n");
        return state;
    }

    printf("SX1262 initialized successfully\r\n");

    state = radio.setDio2AsRfSwitch(true);

    if (state != RADIOLIB_ERR_NONE)
    {
        printf(
            "DIO2 RF-switch setup failed: %d\r\n",
            state
        );

        return state;
    }

    printf("DIO2 RF switch configured\r\n");

    state = node.beginOTAA(
        JOIN_EUI,
        DEV_EUI,
        NWK_KEY,
        APP_KEY
    );

    if (state != RADIOLIB_ERR_NONE)
    {
        printf(
            "beginOTAA failed: %d\r\n",
            state
        );

        return state;
    }

    printf("Sending LoRaWAN join request...\r\n");

    state = node.activateOTAA();

    if (
        state != RADIOLIB_LORAWAN_NEW_SESSION &&
        state != RADIOLIB_LORAWAN_SESSION_RESTORED
    )
    {
        printf(
            "LoRaWAN join failed: %d\r\n",
            state
        );

        return state;
    }

    lora_joined = true;

    printf("LoRaWAN joined successfully\r\n");

    return 0;
}

extern "C" int lora_send(const char *message)
{
    /*
     * Check whether the message pointer is valid.
     */
    if (message == nullptr)
    {
        return -1;
    }

    /*
     * Do not send before the device has joined
     * The Things Stack.
     */
    if (!lora_joined)
    {
        printf("LoRaWAN has not joined yet\r\n");
        return -2;
    }

    /*
     * Send the text as an unconfirmed LoRaWAN uplink
     * using FPort 1.
     */
    int16_t state = node.sendReceive(
        message,
        1,
        false
    );

    /*
     * Check the result.
     */
    if (state != RADIOLIB_ERR_NONE)
    {
        printf(
            "LoRaWAN send failed: %d\r\n",
            state
        );

        return state;
    }

    /*
     * The message was transmitted successfully.
     */
    printf(
        "LoRaWAN message sent: %s\r\n",
        message
    );

    return 0;
}

extern "C" int lora_receive(
    char *received_message,
    int maximum_length
)
{
    /*
     * Check the output buffer.
     */
    if (received_message == nullptr)
    {
        return -1;
    }

    /*
     * Check the buffer size.
     */
    if (maximum_length <= 0)
    {
        return -2;
    }

    /*
     * Downlink receiving is not implemented yet.
     * Return an empty string.
     */
    received_message[0] = '\0';

    /*
     * Zero means that no message was received.
     */
    return 0;
}