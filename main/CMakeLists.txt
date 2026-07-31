idf_component_register(
    SRCS
        "lora_wrapper.cpp"

    INCLUDE_DIRS
        "."

    REQUIRES
        jgromes__radiolib
        esp_driver_gpio
        esp_driver_spi
        esp_timer
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    RADIOLIB_VERBOSE_ASSERT=1
    RADIOLIB_DEBUG_BASIC=1
)