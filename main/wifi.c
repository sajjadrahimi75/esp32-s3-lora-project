#include "wifi.h"
#include "camera.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_eap_client.h"

//#define WIFI_SSID       "test"
//#define WIFI_PASSWORD   "123456789"

//#define POLITO_SSID         "polito"
//#define POLITO_IDENTITY     "s329206@studenti.polito.it"
//#define POLITO_USERNAME     "s329206@studenti.polito.it"
//#define POLITO_PASSWORD     "*******"


#define NTFY_URL \
    "https://ntfy.sh/sim800_myproject_alert_789"

#define WIFI_CONNECTED  BIT0
#define WIFI_FAILED     BIT1

#define WIFI_MAX_RETRY  5
#define WIFI_TX_BLOCK_SIZE  8192


static const char *TAG = "WIFI";

static EventGroupHandle_t wifi_events = NULL;
static int retry_count = 0;
static bool wifi_initialized = false;
static bool time_synchronized = false;
static bool wifi_manual_stop = false;
static bool wifi_started = false;
static esp_netif_t *wifi_sta_netif = NULL;
char wifi_received_message_raw[1024] = {0};
char wifi_received_message[256] = {0};

extern char polito_password[];
extern char polito_username[];
extern char polito_ssid[];
extern char wifi_hotspot_username[];
extern char wifi_hotspot_password[];

int wifi_select=3;
int wifi_error=1;

static esp_http_client_handle_t ntfy_rx_client = NULL;
/*
 * Message is created/updated in main.c.
 */
extern char wifi_message[];

/*
 * =========================================================
 * WIFI EVENT HANDLER
 * =========================================================
 */
static void wifi_turn_off(void)
{
    wifi_manual_stop = true;

    if (wifi_events != NULL)
    {
        xEventGroupClearBits(
            wifi_events,
            WIFI_CONNECTED | WIFI_FAILED
        );
    }

    if (wifi_started)
    {
        esp_err_t err = esp_wifi_stop();

        if (err == ESP_OK)
        {
            wifi_started = false;
        }
        else
        {
            ESP_LOGW(
                TAG,
                "Wi-Fi stop failed: %s",
                esp_err_to_name(err)
            );
        }
    }
}




static void wifi_event_handler(
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

        if (wifi_events != NULL)
        {
            xEventGroupClearBits(
                wifi_events,
                WIFI_CONNECTED
            );
        }

        if (wifi_manual_stop)
        {
            return;
        }

        if (retry_count < WIFI_MAX_RETRY)
        {
            retry_count++;

            ESP_LOGW(
                TAG,
                "Wi-Fi disconnected, retry %d/%d",
                retry_count,
                WIFI_MAX_RETRY
            );

            esp_wifi_connect();
        }
        else
        {
            if (wifi_events != NULL)
            {
                xEventGroupSetBits(wifi_events,WIFI_FAILED);
                    
            }
        }
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        retry_count = 0;

        if (wifi_events != NULL)
        {
            xEventGroupSetBits(
                wifi_events,
                WIFI_CONNECTED
            );
        }

        ESP_LOGI(
            TAG,
            "Wi-Fi got IP address"
        );
    }
}


/*
 * =========================================================
 * CONNECT WIFI
 * =========================================================
 */
/////////////////////////////////////polito connection
static esp_err_t wifi_try_polito(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Selected Wi-Fi: PoliTO");

    retry_count = 0;

    xEventGroupClearBits(
        wifi_events,
        WIFI_CONNECTED | WIFI_FAILED
    );

    wifi_config_t wifi_config = {0};

    /*strcpy(
        (char *)wifi_config.sta.ssid,
        POLITO_SSID
    );*/

    strcpy(
        (char *)wifi_config.sta.ssid,polito_ssid);

    wifi_config.sta.password[0] = '\0';

    

    /*err = esp_eap_client_set_identity(
        (const unsigned char *)POLITO_IDENTITY,
        strlen(POLITO_IDENTITY)
    );*/

    err = esp_eap_client_set_identity(
        (const unsigned char *)polito_username,
        strlen(polito_username)
    );

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
        
    }

    /*err = esp_eap_client_set_username(
        (const unsigned char *)POLITO_USERNAME,
        strlen(POLITO_USERNAME)
    );*/

    err = esp_eap_client_set_username(
        (const unsigned char *)polito_username,
        strlen(polito_username)
    );

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
    
    }

    /*err = esp_eap_client_set_password(
        (const unsigned char *)POLITO_PASSWORD,
        strlen(POLITO_PASSWORD)
    );*/

    err = esp_eap_client_set_password(
        (const unsigned char *)polito_password,
        strlen(polito_password)
    );


    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
        
    }

    err = esp_wifi_sta_enterprise_enable();

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }

    err = esp_wifi_connect();

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }

    EventBits_t bits =
        xEventGroupWaitBits(
            wifi_events,
            WIFI_CONNECTED | WIFI_FAILED,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(20000)
        );

    if (bits & WIFI_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected to PoliTO Wi-Fi");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "PoliTO Wi-Fi connection failed");

    return ESP_FAIL;
}

static esp_err_t wifi_try_hotspot(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Trying hotspot Wi-Fi...");

    /*
     * Stop any pending connection attempt.
     */
    esp_wifi_disconnect();

    /*
     * Disable WPA2 Enterprise before using
     * normal WPA2-PSK hotspot.
     */
    esp_wifi_sta_enterprise_disable();

    retry_count = 0;

    xEventGroupClearBits(
        wifi_events,
        WIFI_CONNECTED | WIFI_FAILED
    );

    /*wifi_config_t wifi_config = {
        .sta = {
            .ssid = wifi_hotspot_username,
            .password = wifi_hotspot_password,
        },
    };*/
     wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        wifi_hotspot_username,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        wifi_hotspot_password,
        sizeof(wifi_config.sta.password) - 1
    );

    err = esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config
    );

    

    err = esp_wifi_connect();

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }

    EventBits_t bits =
        xEventGroupWaitBits(
            wifi_events,
            WIFI_CONNECTED | WIFI_FAILED,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(20000)
        );

    if (bits & WIFI_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected to hotspot");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Hotspot connection failed");

    return ESP_FAIL;
}

/*
 * =========================================================
 * TIME SYNCHRONIZATION
 * =========================================================
 */

static esp_err_t wifi_sync_time(void)
{
    if (time_synchronized)
    {
        return ESP_OK;
    }


    esp_sntp_config_t time_config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG(
            "pool.ntp.org"
        );


    esp_err_t err =
        esp_netif_sntp_init(
            &time_config
        );


    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }


    err =
        esp_netif_sntp_sync_wait(
            pdMS_TO_TICKS(10000)
        );


    if (err != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "SNTP synchronization timeout"
        );

        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }


    time_synchronized = true;


    ESP_LOGI(
        TAG,
        "Time synchronized"
    );


    return ESP_OK;
}




void wifi_full_deinit(void)
{
    ESP_LOGI(TAG, "FULL WIFI RESET START");

    /*
     * 1. Close old NTFY HTTP client
     */
    if (ntfy_rx_client != NULL)
    {
        esp_http_client_close(ntfy_rx_client);
        esp_http_client_cleanup(ntfy_rx_client);

        ntfy_rx_client = NULL;
    }


    /*
     * 2. Prevent automatic reconnect while
     * we intentionally shut WiFi down.
     */
    wifi_manual_stop = true;


    /*
     * 3. Clear old event bits
     */
    if (wifi_events != NULL)
    {
        xEventGroupClearBits(
            wifi_events,
            WIFI_CONNECTED | WIFI_FAILED
        );
    }


    /*
     * 4. Disconnect from current AP
     */
    if (wifi_initialized)
    {
        esp_wifi_disconnect();

        vTaskDelay(pdMS_TO_TICKS(100));
    }


    /*
     * 5. Stop WiFi
     */
    if (wifi_started)
    {
        esp_wifi_stop();

        wifi_started = false;

        vTaskDelay(pdMS_TO_TICKS(100));
    }


    /*
     * 6. Unregister event handlers
     */
    if (wifi_initialized)
    {
        esp_event_handler_unregister(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler
        );

        esp_event_handler_unregister(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_event_handler
        );
    }


    /*
     * 7. Deinitialize WiFi driver
     */
    if (wifi_initialized)
    {
        esp_wifi_deinit();

        wifi_initialized = false;
    }


    /*
     * 8. Destroy old STA netif
     */
    if (wifi_sta_netif != NULL)
    {
        esp_netif_destroy_default_wifi(
            wifi_sta_netif
        );

        wifi_sta_netif = NULL;
    }


    /*
     * 9. Delete old EventGroup
     */
    if (wifi_events != NULL)
    {
        vEventGroupDelete(wifi_events);

        wifi_events = NULL;
    }


    /*
     * 10. Reset internal variables
     */
    retry_count = 0;

    wifi_error = 1;

    esp_netif_sntp_deinit();
    time_synchronized = false;

    wifi_manual_stop = false;

    ESP_LOGI(TAG, "FULL WIFI RESET COMPLETE");
}

//////////////////////////////////////////////////////
esp_err_t wifi_connect_internal(void)
{
    esp_err_t err;


    /*
     * Already initialized
     */
    if (wifi_initialized)
    {
        EventBits_t current_bits =
    xEventGroupGetBits(wifi_events);

        if (current_bits & WIFI_CONNECTED)
        {
            ESP_LOGI(
                TAG,
                "WiFi already connected"
            );
            return ESP_OK;
        }

        retry_count = 0;

        xEventGroupClearBits(
            wifi_events,
            WIFI_CONNECTED | WIFI_FAILED
        );

        /*
        * Wi-Fi was stopped after the previous communication.
        * Start it again.
        */
        if (!wifi_started)
        {
            err = esp_wifi_start();

            if (err != ESP_OK)
            {
                wifi_error = 1;
                //wifi_turn_off();
                return err;
            }

            wifi_started = true;

            /*
            * Do NOT call esp_wifi_connect() here.
            * WIFI_EVENT_STA_START will call it automatically
            * in wifi_event_handler().
            */
        }
        else
        {
            err = esp_wifi_connect();

            if (err != ESP_OK)
            {
                wifi_error = 1;
                wifi_turn_off();
                return err;
            }
        }

        EventBits_t bits =
            xEventGroupWaitBits(
                wifi_events,
                WIFI_CONNECTED | WIFI_FAILED,
                pdFALSE,
                pdFALSE,
                pdMS_TO_TICKS(20000)
            );

        if (bits & WIFI_CONNECTED)
        {
            
            return ESP_OK;
        }

        wifi_error = 1;
        wifi_turn_off();
        return ESP_FAIL;
    }

    

    /*
     * NVS
     */
    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        err = nvs_flash_erase();

    if (err != ESP_OK)
    {
        wifi_error = 1;
        wifi_turn_off();
        return err;
    }

    err = nvs_flash_init();

            err = nvs_flash_init();
        }

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "NVS init failed: %s",
            esp_err_to_name(err)
        );

        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }


    /*
     * Network stack
     */
    err = esp_netif_init();

    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
        
    }


    /*
     * Event loop
     */
    err = esp_event_loop_create_default();

    if (err != ESP_OK &&
        err != ESP_ERR_INVALID_STATE)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
    
    }


    /*
     * Event group
     */
    wifi_events = xEventGroupCreate();

    if (wifi_events == NULL)
    {
        return ESP_ERR_NO_MEM;
    }


    /*
     * Wi-Fi station interface
     */
    /*if (esp_netif_create_default_wifi_sta() == NULL)
    {
        wifi_error = 1;
        wifi_turn_off();
        return ESP_FAIL;
    }*/

    wifi_sta_netif = esp_netif_create_default_wifi_sta();

    if (wifi_sta_netif == NULL)
    {
        ESP_LOGE(TAG, "Failed to create WiFi STA netif");

        wifi_error = 1;

        return ESP_FAIL;
    }




    /*
     * Wi-Fi initialization
     */
    wifi_init_config_t wifi_init_config =
        WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(
        &wifi_init_config
    );

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }


    /*
     * Register events
     */
    err =
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler,
            NULL
        );

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }


    err =
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_event_handler,
            NULL
        );

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
    }


 


    err =
        esp_wifi_set_storage(
            WIFI_STORAGE_RAM
        );

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
    }


    err =
        esp_wifi_set_mode(
            WIFI_MODE_STA
        );

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
    }

    wifi_config_t wifi_config = {0};

    if (wifi_select == 1)
    {
        ESP_LOGI(TAG, "Selected Wi-Fi: PoliTO");

        /*strcpy(
            (char *)wifi_config.sta.ssid,
            POLITO_SSID
        );*/

        strcpy(
            (char *)wifi_config.sta.ssid,
            polito_ssid
        );

        wifi_config.sta.password[0] = '\0';

        err = esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        );

        if (err != ESP_OK)
        {
            wifi_error=1;
            wifi_turn_off();
            return err;
        }

        err = esp_eap_client_set_identity(
            (const unsigned char *)polito_username,
            strlen(polito_username)
        );

        if (err != ESP_OK)
        {
            wifi_error=1;
            wifi_turn_off();
            return err;
        }

        err = esp_eap_client_set_username(
            (const unsigned char *)polito_username,
            strlen(polito_username)
        );

        if (err != ESP_OK)
        {
            wifi_error=1;
            wifi_turn_off();
            return err;
           
        }

        err = esp_eap_client_set_password(
            (const unsigned char *)polito_password,
            strlen(polito_password)
        );

        if (err != ESP_OK)
        {
            wifi_error=1;
            wifi_turn_off();
            return err;
           
        }

        err = esp_wifi_sta_enterprise_enable();

        if (err != ESP_OK)
        {
            wifi_error=1;
            wifi_turn_off();
            return err;
           
        }
    }
    else if (wifi_select == 2 || wifi_select == 3)
    {
        ESP_LOGI(TAG, "Selected Wi-Fi: Hotspot");

        esp_wifi_sta_enterprise_disable();

        if(wifi_select==3)
        {
            snprintf(wifi_hotspot_username,sizeof(wifi_hotspot_username),"test");
            snprintf(wifi_hotspot_password,sizeof(wifi_hotspot_password),"123456789");
        }

        strcpy((char *)wifi_config.sta.ssid,wifi_hotspot_username);

        strcpy((char *)wifi_config.sta.password,wifi_hotspot_password);

        err = esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        );

        if (err != ESP_OK)
        {
            wifi_error=1;
            wifi_turn_off();
            return err;
        
        }
    }
    else
    {
        ESP_LOGE(
            TAG,
            "Invalid wifi_select = %d",
            wifi_select
        );

        return ESP_ERR_INVALID_ARG;
    }



    /*
     * Start Wi-Fi
     */
    err =
        esp_wifi_start();

    if (err != ESP_OK)
    {
        wifi_error=1;
        wifi_turn_off();
        return err;
    }
    
    /*
    * Wi-Fi driver and netif are now initialized.
    * Even if connection fails later, do not initialize them again.
    */
    wifi_initialized = true;
    wifi_started = true;

    /*
     * Disable Wi-Fi power save
     */
    esp_wifi_set_ps(
        WIFI_PS_NONE
    );


    /*
     * Wait for connection
     */
    EventBits_t bits =
        xEventGroupWaitBits(
            wifi_events,
            WIFI_CONNECTED | WIFI_FAILED,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(20000)
        );


    if (!(bits & WIFI_CONNECTED))
    {
        ESP_LOGE(
            TAG,
            "Wi-Fi connection failed"
        );

        return ESP_FAIL;
    }


   


    ESP_LOGI(TAG,"Wi-Fi connected successfully");

    wifi_sync_time();
    return ESP_OK;
}





/*
 * =========================================================
 * SEND WIFI MESSAGE
 * =========================================================
 */
static esp_err_t wifi_send_message(void)
{

    ESP_LOGI(
        TAG,
        "Sending Wi-Fi message..."
    );


    /*
     * HTTPS configuration
     */
    esp_http_client_config_t http_config = {

        .url = NTFY_URL,

        .crt_bundle_attach =
            esp_crt_bundle_attach,

        .timeout_ms = 120000,

        .buffer_size = 4096,

        .buffer_size_tx = 16384,

        .keep_alive_enable = true,
    };


    esp_http_client_handle_t client =
        esp_http_client_init(
            &http_config
        );


    if (client == NULL)
    {
        ESP_LOGE(
            TAG,
            "HTTP client init failed for message"
        );
        do_not_send_text=0;
        wifi_error=1;
        wifi_turn_off();
        return ESP_FAIL;
    }


    /*
     * POST text message
     */
    esp_http_client_set_method(
        client,
        HTTP_METHOD_POST
    );


    esp_http_client_set_header(
        client,
        "Title",
        "ESP32 Camera Alert"
    );

    esp_http_client_set_header(
        client,
        "Content-Type",
        "text/plain"
    );


    size_t message_length =
        strlen(wifi_message);


    esp_err_t err =
        esp_http_client_open(
            client,
            message_length
        );


    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Message HTTPS open failed: %s",
            esp_err_to_name(err)
        );

        esp_http_client_cleanup(
            client
        );
        do_not_send_text=0;
        wifi_error=1;
        wifi_turn_off();
        return err;
    }


    size_t offset = 0;


    while (offset < message_length)
    {
        int written =
            esp_http_client_write(
                client,
                wifi_message + offset,
                (int)(message_length - offset)
            );


        if (written <= 0)
        {
            ESP_LOGE(
                TAG,
                "Message send failed at %u / %u bytes",
                (unsigned)offset,
                (unsigned)message_length
            );

            esp_http_client_close(
                client
            );

            esp_http_client_cleanup(
                client
            );
            do_not_send_text=0;
            wifi_error=1;
            wifi_turn_off();
            return ESP_FAIL;
        }


        offset +=
            (size_t)written;
    }


    int64_t response_length =
        esp_http_client_fetch_headers(
            client
        );


    int status =
        esp_http_client_get_status_code(
            client
        );


    ESP_LOGI(
        TAG,
        "MESSAGE HTTP STATUS = %d",
        status
    );


    char response_body[256] = {0};


    int response_read =
        esp_http_client_read_response(
            client,
            response_body,
            sizeof(response_body) - 1
        );


    if (response_read > 0)
    {
        response_body[response_read] =
            '\0';


        ESP_LOGI(
            TAG,
            "NTFY MESSAGE RESPONSE: %s",
            response_body
        );
    }


    esp_http_client_close(
        client
    );


    esp_http_client_cleanup(
        client
    );


    if (response_length < 0)
    {
        ESP_LOGE(
            TAG,
            "Invalid message HTTP response"
        );
        do_not_send_text=0;
        wifi_error=1;
        wifi_turn_off();
        return ESP_FAIL;
    }


    if (status < 200 ||
        status >= 300)
    {
        ESP_LOGE(
            TAG,
            "Server rejected message, status=%d",
            status
        );
        do_not_send_text=0;
        wifi_error=1;
        wifi_turn_off();
        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "WIFI MESSAGE SENT SUCCESSFULLY"
    );

    do_not_send_text=1;
    return ESP_OK;

}


/*
 * =========================================================
 * SEND PHOTO FROM PSRAM
 * =========================================================
 */

static esp_err_t wifi_send_photo_from_psram(void)
{
    /*
     * These variables come from camera.c:
     *
     * photo_psram_address
     * photo_psram_size
     */

    if (photo_psram_address == NULL ||
        photo_psram_size == 0)
    {
        ESP_LOGE(
            TAG,
            "No photo available"
        );

        ESP_LOGE(
            TAG,
            "Address = %p",
            photo_psram_address
        );

        ESP_LOGE(
            TAG,
            "Size = %u",
            (unsigned)photo_psram_size
        );

        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "PHOTO FOUND IN PSRAM"
    );

    ESP_LOGI(
        TAG,
        "PSRAM ADDRESS = %p",
        photo_psram_address
    );

    ESP_LOGI(
        TAG,
        "PHOTO SIZE = %u bytes",
        (unsigned)photo_psram_size
    );


    /*
     * HTTPS configuration
     */
    esp_http_client_config_t http_config = {

        .url = NTFY_URL,

        .crt_bundle_attach =
            esp_crt_bundle_attach,

        .timeout_ms = 120000,

        .buffer_size = 4096,

        .buffer_size_tx = 16384,

        .keep_alive_enable = false,
    };


    esp_http_client_handle_t client =
        esp_http_client_init(
            &http_config
        );


    if (client == NULL)
    {
        ESP_LOGE(
            TAG,
            "HTTP client init failed"
        );

        return ESP_FAIL;
    }


    /*
     * PUT request
     */
    esp_http_client_set_method(
        client,
        HTTP_METHOD_PUT
    );


    /*
     * ntfy headers
     */
    esp_http_client_set_header(
        client,
        "Filename",
        "camera.jpg"
    );

    esp_http_client_set_header(
        client,
        "Title",
        "ESP32 Camera"
    );

    esp_http_client_set_header(
        client,
        "Content-Type",
        "image/jpeg"
    );


    /*
     * Open HTTPS with exact JPEG size.
     */
    esp_err_t err =
        esp_http_client_open(
            client,
            photo_psram_size
        );


    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "HTTPS open failed: %s",
            esp_err_to_name(err)
        );

        esp_http_client_cleanup(
            client
        );

        wifi_error=1;
        wifi_turn_off();
        return err;
       
    }


    ESP_LOGI(
        TAG,
        "Sending JPEG from PSRAM..."
    );


    /*
     * Send directly from PSRAM
     */
    size_t offset = 0;


    while (offset < photo_psram_size)
    {
        size_t remaining =
            photo_psram_size - offset;


        size_t block_size =
            remaining > WIFI_TX_BLOCK_SIZE
            ? WIFI_TX_BLOCK_SIZE
            : remaining;


        int written =
            esp_http_client_write(
                client,
                (const char *)
                (photo_psram_address + offset),
                (int)block_size
            );


        if (written <= 0)
        {
            ESP_LOGE(
                TAG,
                "JPEG send failed at %u / %u bytes",
                (unsigned)offset,
                (unsigned)photo_psram_size
            );

            esp_http_client_close(
                client
            );

            esp_http_client_cleanup(
                client
            );

            return ESP_FAIL;
        }


        offset +=
            (size_t)written;
    }


    ESP_LOGI(
        TAG,
        "JPEG DATA SENT = %u bytes",
        (unsigned)offset
    );


    /*
     * Get server response
     */
    int64_t response_length =
        esp_http_client_fetch_headers(
            client
        );


    int status =
        esp_http_client_get_status_code(
            client
        );


    ESP_LOGI(
        TAG,
        "HTTP STATUS = %d",
        status
    );


    /*
     * Optional response body
     */
    char response_body[256] = {0};


    int response_read =
        esp_http_client_read_response(
            client,
            response_body,
            sizeof(response_body) - 1
        );


    if (response_read > 0)
    {
        response_body[response_read] =
            '\0';


        ESP_LOGI(
            TAG,
            "NTFY RESPONSE: %s",
            response_body
        );
    }


    /*
     * Close HTTPS
     */
    esp_http_client_close(
        client
    );


    esp_http_client_cleanup(
        client
    );


    /*
     * Check HTTP result
     */
    if (response_length < 0)
    {
        ESP_LOGE(
            TAG,
            "Invalid HTTP response"
        );

        return ESP_FAIL;
    }


    if (status < 200 ||
        status >= 300)
    {
        ESP_LOGE(
            TAG,
            "Server rejected photo, status=%d",
            status
        );

        return ESP_FAIL;
    }


    ESP_LOGI(
        TAG,
        "PHOTO SENT SUCCESSFULLY"
    );


    return ESP_OK;
}




/*
 * =========================================================
 * PUBLIC FUNCTION
 * =========================================================
 */

esp_err_t wifi_receive(void)
{

    wifi_manual_stop = false;
    wifi_error = 0;
    esp_err_t err;
    /*
     * Connect WiFi
     */
    #if 0
    esp_err_t err = wifi_connect_internal();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi connection failed");

        wifi_error = 1;
        return ESP_FAIL;
    }


    /*
     * Synchronize time for HTTPS
     */
    err = wifi_sync_time();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Time sync failed");

        wifi_error = 1;
        return ESP_FAIL;
    }
        #endif


    /*
     * Receive message from ntfy
     */
    char receive_url[256];

    snprintf(
        receive_url,
        sizeof(receive_url),
        "%s/json?poll=1&since=latest",
        NTFY_URL
    );


    if (ntfy_rx_client == NULL)
    {
        esp_http_client_config_t config = {
            .url = receive_url,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .timeout_ms = 30000,
            .buffer_size = 4096,
            .buffer_size_tx = 16384,

            .keep_alive_enable = true,
            .keep_alive_idle = 5,
            .keep_alive_interval = 5,
            .keep_alive_count = 3,
        };

        ntfy_rx_client = esp_http_client_init(&config);

        if (ntfy_rx_client == NULL)
        {
            ESP_LOGE(TAG, "HTTP client init failed");
            return ESP_FAIL;
        }

        esp_http_client_set_method(
            ntfy_rx_client,
            HTTP_METHOD_GET
        );
    }


    err = esp_http_client_open(ntfy_rx_client,0);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "NTFY receive open failed: %s",
            esp_err_to_name(err)
        );

        //esp_http_client_cleanup(client);
        return ESP_FAIL;
    }


    int64_t response_length =
        esp_http_client_fetch_headers(ntfy_rx_client);


    int status =
        esp_http_client_get_status_code(ntfy_rx_client);


    if (response_length < 0 ||status < 200 || status >= 300)
    {
        ESP_LOGE(
            TAG,
            "NTFY receive failed, status=%d",
            status
        );

        esp_http_client_close(ntfy_rx_client);
        //esp_http_client_cleanup(client);

        return ESP_FAIL;
    }


    int received =
    esp_http_client_read_response(
        ntfy_rx_client,
        wifi_received_message_raw,
        sizeof(wifi_received_message_raw) - 1
    );


    if (received <= 0)
    {
        ESP_LOGE(TAG, "No ntfy data received");

        esp_http_client_close(ntfy_rx_client);
        esp_http_client_clear_response_buffer(ntfy_rx_client);
        //esp_http_client_cleanup(client);

        return ESP_FAIL;
    }


    wifi_received_message_raw[received] = '\0';

        cJSON *root = cJSON_Parse(wifi_received_message_raw);

    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse ntfy JSON");

        esp_http_client_close(ntfy_rx_client);
        esp_http_client_clear_response_buffer(ntfy_rx_client);
        //esp_http_client_cleanup(client);

        return ESP_FAIL;
    }

    cJSON *message =
        cJSON_GetObjectItemCaseSensitive(
            root,
            "message"
        );

    if (!cJSON_IsString(message) ||
        message->valuestring == NULL)
    {
        ESP_LOGE(TAG, "No message field in ntfy JSON");

        cJSON_Delete(root);

        esp_http_client_close(ntfy_rx_client);
        esp_http_client_clear_response_buffer(ntfy_rx_client);
        //esp_http_client_cleanup(client);

        return ESP_FAIL;
    }

    strncpy(
        wifi_received_message,
        message->valuestring,
        sizeof(wifi_received_message) - 1
    );

    wifi_received_message[
        sizeof(wifi_received_message) - 1
    ] = '\0';

    cJSON_Delete(root);

    ESP_LOGI(
        TAG,
        "NTFY MESSAGE ONLY: %s",
        wifi_received_message
    );


    ESP_LOGI(
        TAG,
        "NTFY RX: %s",
        wifi_received_message_raw
    );


    /*
     * Close HTTP only.
     * DO NOT turn WiFi off.
     */
    //esp_http_client_close(client);
    //esp_http_client_cleanup(client);

    for (int i = 0; wifi_received_message[i] != '\0'; i++)
    {
        wifi_received_message[i] =
            tolower((unsigned char)wifi_received_message[i]);
    }
    wifi_error = 0;

    return ESP_OK;
}



esp_err_t wifi_message_send(void)
{
    wifi_manual_stop = false;
    wifi_error = 0;

    esp_err_t err = wifi_connect_internal();

    if (err != ESP_OK)
    {
        wifi_error = 1;
        return ESP_FAIL;
    }

    err = wifi_sync_time();

    if (err != ESP_OK)
    {
        wifi_error = 1;
        return ESP_FAIL;
    }

    err = wifi_send_message();

    if (err != ESP_OK)
    {
        wifi_error = 1;
        return ESP_FAIL;
    }

    wifi_error = 0;
    return ESP_OK;
}

esp_err_t wifi_photo_send(void)
{
    wifi_manual_stop = false;
    wifi_error = 0;

    esp_err_t err = wifi_connect_internal();

    if (err != ESP_OK)
    {
        wifi_error = 1;
        return ESP_FAIL;
    }

    err = wifi_sync_time();

    if (err != ESP_OK)
    {
        wifi_error = 1;
        return ESP_FAIL;
    }

    if (photo_psram_address == NULL ||
        photo_psram_size == 0)
    {
        wifi_error = 1;
        return ESP_FAIL;
    }

    err = wifi_send_photo_from_psram();

    if (err != ESP_OK)
    {
        wifi_error = 1;
        return ESP_FAIL;
    }

    wifi_error = 0;
    return ESP_OK;
}