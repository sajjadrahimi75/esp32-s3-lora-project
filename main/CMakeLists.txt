idf_component_register(
    SRCS
        "lora_wrapper.cpp"
        "main.c"
        "GSM.c"
        "GSM_PPP.c"
        "camera.c"
        "wifi.c"
        "image_stream.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        jgromes__radiolib
        esp_modem
        esp_netif
        esp_event
        esp_http_client
        esp_driver_gpio
        esp_driver_uart
        esp_driver_spi
        esp_timer
        
        esp_wifi
        esp_netif
        esp_event
        esp_http_client
        nvs_flash
        mbedtls
        esp_driver_pcnt
        esp_psram
        esp_ringbuf

)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    RADIOLIB_VERBOSE_ASSERT=1
    RADIOLIB_DEBUG_BASIC=1
    RADIOLIB_DEBUG_PROTOCOL=1
    RADIOLIB_DEBUG_SPI=1
)