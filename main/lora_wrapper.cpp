#include <stdio.h>

#include "lora_wrapper.h"

extern "C" int lora_init(void)
{
    printf("C++ LoRa wrapper started\n");
    return 0;
}

extern "C" int lora_send(const char *message)
{
    if (message == nullptr)
    {
        return -1;
    }

    printf("C++ received message: %s\n", message);
    return 0;
}