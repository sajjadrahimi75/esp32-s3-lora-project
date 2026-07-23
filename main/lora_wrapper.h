#ifndef LORA_WRAPPER_H
#define LORA_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the SX1262 and join
 * The Things Stack using OTAA.
 */
int lora_init(void);

/*
 * Send one LoRaWAN uplink message.
 */
int lora_send(const char *message);

/*
 * Check for a received LoRaWAN message.
 *
 * Downlink support is not implemented yet.
 */
int lora_receive(
    char *received_message,
    int maximum_length
);

#ifdef __cplusplus
}
#endif

#endif