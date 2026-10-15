
#include "wifi.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_SSID       "test"
#define WIFI_PASSWORD   "123456789"

#define NTFY_URL \
    "https://ntfy.sh/sim800_myproject_alert_789"

#define WIFI_CONNECTED BIT0
#define WIFI_FAILED    BIT1

static const char *TAG = "WIFI_NTFY";

static EventGroupHandle_t wifi_events;
static int retry_count = 0;

static esp_http_client_handle_t stream_client = NULL;
// Wi-Fi connection events
static void wifi_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(
            wifi_events,
            WIFI_CONNECTED
        );

        if (retry_count < 5)
        {
            retry_count++;
            esp_wifi_connect();
        }
        else
        {
            xEventGroupSetBits(wifi_events, WIFI_FAILED);
        }
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        retry_count = 0;

        xEventGroupSetBits(
            wifi_events,
            WIFI_CONNECTED
        );
    }
}


// Connect to Wi-Fi
static esp_err_t wifi_connect(void)
{
    esp_err_t err = nvs_flash_init();

    if (err != ESP_OK)
        return err;

    err = esp_netif_init();

    if (err != ESP_OK)
        return err;

    err = esp_event_loop_create_default();

    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }

    wifi_events = xEventGroupCreate();

    if (wifi_events == NULL)
        return ESP_ERR_NO_MEM;

    if (esp_netif_create_default_wifi_sta() == NULL)
        return ESP_FAIL;

    wifi_init_config_t wifi_cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&wifi_cfg);

    if (err != ESP_OK)
        return err;

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_handler,
            NULL
        )
    );

    wifi_config_t config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_storage(WIFI_STORAGE_RAM)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(WIFI_IF_STA, &config)
    );

    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        wifi_events,
        WIFI_CONNECTED | WIFI_FAILED,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(20000)
    );

    if (!(bits & WIFI_CONNECTED))
    {
        ESP_LOGE(TAG, "Wi-Fi connection failed");

        esp_wifi_stop();
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Wi-Fi connected successfully");

    return ESP_OK;
}


// Send JPEG to ntfy
// Start Wi-Fi + HTTPS stream
esp_err_t wifi_stream_start(void)
{
    ESP_LOGI(TAG, "Connecting to Wi-Fi: %s", WIFI_SSID);

    esp_err_t err = wifi_connect();

    if (err != ESP_OK)
    {
        return err;
    }

    // Disable Wi-Fi power saving for faster streaming
    esp_wifi_set_ps(WIFI_PS_NONE);


    // -----------------------------
    // SNTP time synchronization
    // -----------------------------
    esp_sntp_config_t time_config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    err = esp_netif_sntp_init(&time_config);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "SNTP init failed: %s",
                 esp_err_to_name(err));
    }
    else
    {
        err = esp_netif_sntp_sync_wait(
            pdMS_TO_TICKS(5000)
        );

        if (err != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "SNTP timeout - continuing anyway");
        }
        else
        {
            ESP_LOGI(TAG,
                     "Time synchronized");
        }
    }


    // -----------------------------
    // HTTPS client
    // -----------------------------
    esp_http_client_config_t config = {
        .url = NTFY_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 60000,
        .buffer_size = 4096,
        .buffer_size_tx = 4096,
    };

    stream_client =
        esp_http_client_init(&config);

    if (stream_client == NULL)
    {
        ESP_LOGE(TAG, "HTTP client init failed");
        return ESP_FAIL;
    }

    esp_http_client_set_method(
        stream_client,
        HTTP_METHOD_PUT
    );

    esp_http_client_set_header(
        stream_client,
        "Filename",
        "camera.jpg"
    );

    esp_http_client_set_header(
        stream_client,
        "Title",
        "ESP32 Camera"
    );

    esp_http_client_set_header(
        stream_client,
        "Content-Type",
        "image/jpeg"
    );

    /*
     * JPEG final size is unknown.
     * Use chunked HTTP transfer.
     */
    err = esp_http_client_open(
        stream_client,
        -1
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "HTTPS open failed: %s",
                 esp_err_to_name(err));

        esp_http_client_cleanup(stream_client);
        stream_client = NULL;

        return err;
    }

    ESP_LOGI(TAG, "HTTPS stream ready");

    return ESP_OK;
}


esp_err_t wifi_stream_write(
    const uint8_t *data,
    size_t len)
{
    if (stream_client == NULL)
        return ESP_ERR_INVALID_STATE;

    if (data == NULL || len == 0)
        return ESP_ERR_INVALID_ARG;

    char chunk_header[16];

    int header_len = snprintf(
        chunk_header,
        sizeof(chunk_header),
        "%X\r\n",
        (unsigned int)len
    );

    if (header_len <= 0)
        return ESP_FAIL;


    // 1. Send HTTP chunk length
    int written = esp_http_client_write(
        stream_client,
        chunk_header,
        header_len
    );

    if (written != header_len)
    {
        ESP_LOGE(TAG, "Chunk header write failed");
        return ESP_FAIL;
    }


    // 2. Send JPEG data
    size_t offset = 0;

    while (offset < len)
    {
        written = esp_http_client_write(
            stream_client,
            (const char *)(data + offset),
            (int)(len - offset)
        );

        if (written <= 0)
        {
            ESP_LOGE(TAG, "JPEG chunk write failed");
            return ESP_FAIL;
        }

        offset += written;
    }


    // 3. End this HTTP chunk
    written = esp_http_client_write(
        stream_client,
        "\r\n",
        2
    );

    if (written != 2)
    {
        ESP_LOGE(TAG, "Chunk trailer write failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}


esp_err_t wifi_stream_finish(void)
{
    if (stream_client == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }


    // Final HTTP chunk
    int written = esp_http_client_write(
        stream_client,
        "0\r\n\r\n",
        5
    );

    if (written != 5)
    {
        ESP_LOGE(TAG,
                 "Final chunk terminator failed");

        esp_http_client_cleanup(stream_client);
        stream_client = NULL;

        return ESP_FAIL;
    }


    // Wait for server response
    int64_t response =
        esp_http_client_fetch_headers(stream_client);

    int status =
        esp_http_client_get_status_code(stream_client);

    char response_body[512] = {0};

    int n = esp_http_client_read_response(
        stream_client,
        response_body,
        sizeof(response_body) - 1
    );

    if (n > 0)
    {
        response_body[n] = '\0';

        ESP_LOGI(TAG,
                 "NTFY RESPONSE: %s",
                 response_body);
    }

    ESP_LOGI(TAG,
             "ntfy HTTP status: %d",
             status);


    esp_http_client_cleanup(stream_client);
    stream_client = NULL;

    esp_netif_sntp_deinit();


    if (response < 0 ||
        status < 200 ||
        status >= 300)
    {
        ESP_LOGE(TAG,
                 "ntfy rejected image");

        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "PHOTO STREAM COMPLETE");

    return ESP_OK;
}

