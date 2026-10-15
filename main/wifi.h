#ifndef WIFI_H
#define WIFI_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t wifi_stream_start(void);

esp_err_t wifi_stream_write(
    const uint8_t *data,
    size_t len
);

esp_err_t wifi_stream_finish(void);

#endif