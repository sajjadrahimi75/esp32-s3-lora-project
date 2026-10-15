#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"
#include "esp_camera.h"


extern int photo_quality;
extern int photo_size;


esp_err_t camera_init_ov5640(void);

camera_fb_t *camera_capture(void);

void camera_release(camera_fb_t *fb);
camera_fb_t *camera_take_photo(void);

void camera_power_off(void);


#endif