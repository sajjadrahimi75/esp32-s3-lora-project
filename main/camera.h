#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"
#include "esp_camera.h"


extern int photo_quality;
extern int photo_size;

extern uint8_t *photo_psram_address;
extern size_t photo_psram_size;

esp_err_t camera_init_ov5640(void);

//camera_fb_t *camera_capture(void);

void camera_release(camera_fb_t *fb);
camera_fb_t *camera_take_photo(void);



esp_err_t camera_process(void);
extern int photo_capture_error;
extern int auto_focus;
extern int sharpness;
extern int contrast;
extern int brightness;
extern int saturation;
extern int hmirror;
extern int vmirror;


#endif