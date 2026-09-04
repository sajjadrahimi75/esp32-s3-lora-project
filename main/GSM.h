#ifndef GSM_H
#define GSM_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void gsm_init(const uint8_t *test_image,size_t test_image_len);
//void gsm_send_command(const char *command);
//void gsm_send_sms(const char *phone_number, const char *message);

#ifdef __cplusplus
}
#endif

#endif