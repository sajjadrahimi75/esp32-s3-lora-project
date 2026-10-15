#ifndef IMAGE_STREAM_H
#define IMAGE_STREAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t image_stream_init(void);

size_t image_stream_push(
    const uint8_t *data,
    size_t len
);

bool image_stream_is_finished(void);
void image_stream_frame_end(void);

#endif