#include "gsm.h"

#include <stdio.h>
#include "esp_http_client.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"

#include "esp_modem_api.h"
#include "esp_modem_config.h"


#define GSM_UART       UART_NUM_1
#define GSM_TX_PIN     17
#define GSM_RX_PIN     18

#define PPP_CONNECTED_BIT BIT0
#define PPP_LOST_BIT      BIT1

static const char *TAG = "GSM_PPP";

static EventGroupHandle_t ppp_events;

static esp_modem_dce_t *dce = NULL;
static esp_netif_t *ppp_netif = NULL;


static void on_ip_event(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if(event_id == IP_EVENT_PPP_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "======================");
        ESP_LOGI(TAG, "PPP CONNECTED");
        ESP_LOGI(TAG,
                 "IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "======================");

        xEventGroupSetBits(
            ppp_events,
            PPP_CONNECTED_BIT);
    }

    else if(event_id == IP_EVENT_PPP_LOST_IP)
    {
        ESP_LOGW(TAG, "PPP LOST IP");

        xEventGroupSetBits(
            ppp_events,
            PPP_LOST_BIT);
    }
}


static void gsm_power_on(void)
{
    gpio_reset_pin(GPIO_NUM_4);
    gpio_set_direction(
        GPIO_NUM_4,
        GPIO_MODE_OUTPUT);

    gpio_reset_pin(GPIO_NUM_7);
    gpio_set_direction(
        GPIO_NUM_7,
        GPIO_MODE_OUTPUT);

    /* GSM power rail OFF initially */
    gpio_set_level(GPIO_NUM_4, 0);

    /* PWRKEY released */
    gpio_set_level(GPIO_NUM_7, 1);

    vTaskDelay(pdMS_TO_TICKS(1000));


    /* GSM power rail ON */
    gpio_set_level(GPIO_NUM_4, 1);

    ESP_LOGI(TAG, "GSM POWER ON");

    vTaskDelay(pdMS_TO_TICKS(100));


    /* PWRKEY LOW */
    gpio_set_level(GPIO_NUM_7, 0);

    vTaskDelay(pdMS_TO_TICKS(100));


    /* release PWRKEY */
    gpio_set_level(GPIO_NUM_7, 1);

    ESP_LOGI(TAG, "GSM PWRKEY RELEASED");

    /* Allow SIM800 to boot */
    vTaskDelay(pdMS_TO_TICKS(9000));
}


static esp_err_t gsm_start_ppp(void)
{
    gsm_power_on();


    /*
     * Initialize ESP networking
     */
    ESP_ERROR_CHECK(esp_netif_init());

    esp_err_t err =
        esp_event_loop_create_default();

    if(err != ESP_OK &&
       err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG,
                 "Event loop error: %s",
                 esp_err_to_name(err));

        return err;
    }


    ppp_events = xEventGroupCreate();

    if(ppp_events == NULL)
    {
        return ESP_ERR_NO_MEM;
    }


    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            on_ip_event,
            NULL));


    /*
     * Create PPP interface
     */
    esp_netif_config_t netif_config =
        ESP_NETIF_DEFAULT_PPP();

    ppp_netif =
        esp_netif_new(&netif_config);

    if(ppp_netif == NULL)
    {
        ESP_LOGE(TAG,
                 "Failed to create PPP netif");

        return ESP_FAIL;
    }


    /*
     * APN configuration.
     * Default CID = 1
     * Default PDP type = IP
     */
    esp_modem_dce_config_t dce_config =
        ESP_MODEM_DCE_DEFAULT_CONFIG(
            "apn.fastweb.it");


    /*
     * UART configuration
     */
    esp_modem_dte_config_t dte_config =
        ESP_MODEM_DTE_DEFAULT_CONFIG();

    dte_config.uart_config.port_num =
        GSM_UART;

    dte_config.uart_config.tx_io_num =
        GSM_TX_PIN;

    dte_config.uart_config.rx_io_num =
        GSM_RX_PIN;

    dte_config.uart_config.rts_io_num =
        UART_PIN_NO_CHANGE;

    dte_config.uart_config.cts_io_num =
        UART_PIN_NO_CHANGE;

    dte_config.uart_config.baud_rate =
        115200;

    dte_config.uart_config.flow_control =
        ESP_MODEM_FLOW_CONTROL_NONE;

    dte_config.uart_config.rx_buffer_size =
        4096;

    dte_config.uart_config.tx_buffer_size =
        4096;

    dte_config.dte_buffer_size =
        2048;


    /*
     * Create SIM800 modem
     */
    ESP_LOGI(TAG, "Creating A7682E DCE");

    dce = esp_modem_new(
        &dte_config,
        &dce_config,
        ppp_netif
    );

    if(dce == NULL)
    {
        ESP_LOGE(TAG,
                 "Failed to create SIM800 DCE");

        return ESP_FAIL;
    }


    
////////////////////////Test AT communication
     char at_response[128] = {0};

esp_err_t at_result =
    esp_modem_at(
        dce,
        "AT",
        at_response,
        3000
    );

ESP_LOGI(TAG,
         "AT result = %s",
         esp_err_to_name(at_result));

ESP_LOGI(TAG,
         "AT response = %s",
         at_response);


///////////////////////////////////////////////////
     
    ESP_LOGI(TAG,
             "Synchronizing with SIM800");

    if(esp_modem_sync(dce) != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "SIM800 AT sync failed");

        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "SIM800 AT communication OK");


   char response[128];
int rssi = 0;
int ber = 0;

/* Wait for useful signal */
for(int retry = 0; retry < 30; retry++)
{
    if(esp_modem_get_signal_quality(dce, &rssi, &ber) == ESP_OK)
    {
        ESP_LOGI(TAG, "Signal RSSI=%d BER=%d", rssi, ber);

        if(rssi > 0 && rssi != 99)
        {
            break;
        }
    }

    ESP_LOGW(TAG, "Waiting for GSM signal...");
    vTaskDelay(pdMS_TO_TICKS(1000));
}

    /*
     * Switch SIM800 UART from AT mode
     * to PPP data mode
     */
    ESP_LOGI(TAG,
             "STARTING PPPoS");

    xEventGroupClearBits(
        ppp_events,
        PPP_CONNECTED_BIT |
        PPP_LOST_BIT);

    err =
        esp_modem_set_mode(
            dce,
            ESP_MODEM_MODE_DATA);

    if(err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Cannot enter PPP data mode: %s",
                 esp_err_to_name(err));

        return err;
    }


    /*
     * Wait maximum 60 seconds for IP
     */
    EventBits_t bits =
        xEventGroupWaitBits(
            ppp_events,
            PPP_CONNECTED_BIT |
            PPP_LOST_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(60000));


    if(bits & PPP_CONNECTED_BIT)
    {
        ESP_LOGI(TAG,
                 "PPP INTERNET READY");

        return ESP_OK;
    }


    ESP_LOGE(TAG,
             "PPP CONNECTION FAILED");

    return ESP_FAIL;
}


   static esp_err_t ntfy_send_test(void)
{
    const char *message = "HELLO FROM ESP32 PPPoS";

    esp_http_client_config_t config = {
        .url = "http://ntfy.sh/sim800_myproject_alert_789",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if(client == NULL)
    {
        ESP_LOGE(TAG, "HTTP client creation failed");
        return ESP_FAIL;
    }

    esp_http_client_set_header(
        client,
        "Content-Type",
        "text/plain"
    );

    esp_http_client_set_post_field(
        client,
        message,
        strlen(message)
    );

    ESP_LOGI(TAG, "Sending HELLO to ntfy...");

    esp_err_t err =
        esp_http_client_perform(client);

    if(err == ESP_OK)
    {
        int status =
            esp_http_client_get_status_code(client);

        ESP_LOGI(TAG,
                 "ntfy HTTP status: %d",
                 status);
    }
    else
    {
        ESP_LOGE(TAG,
                 "HTTP request failed: %s",
                 esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    return err;
}




void gsm_ppp_init(
    const uint8_t *test_image,
    size_t test_image_len)
{
    printf("\nSTARTING PPP TEST\n");

    esp_err_t result =
        gsm_start_ppp();

    if(result == ESP_OK)
    {
        printf("\n************************\n");
        printf("PPP CONNECTED SUCCESSFULLY\n");
        printf("ESP32 HAS INTERNET\n");
        printf("************************\n");
        ntfy_send_test();
    }
    else
    {
        printf("\nPPP FAILED\n");
    }




 
    /*
     * DO NOT send image yet.
     *
     * First we prove PPP works.
     */
}