#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"
#include "esp_camera.h"

esp_err_t camera_init_ov5640(void);

camera_fb_t *camera_take_photo(void);

void camera_return_photo(camera_fb_t *fb);

void camera_send_photo_uart(camera_fb_t *fb);

#endif