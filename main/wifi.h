#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
extern char wifi_received_message[256];

esp_err_t wifi_receive(void);
esp_err_t wifi_message_send(void);
esp_err_t wifi_photo_send(void);
esp_err_t wifi_connect_internal(void);
void wifi_full_deinit(void);
extern char wifi_message[300];
extern char polito_password[100];
extern char polito_username[100];
extern char polito_ssid[20];
extern char wifi_hotspot_username[20];
extern char wifi_hotspot_password[20];
extern int wifi_error;
extern int do_not_send_text;
extern int wifi_select;
#endif