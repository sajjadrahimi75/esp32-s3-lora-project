#ifndef LORA_WRAPPER_H
#define LORA_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

int lora_init(void);
int lora_send(const char *message);

#ifdef __cplusplus
}
#endif

#endif