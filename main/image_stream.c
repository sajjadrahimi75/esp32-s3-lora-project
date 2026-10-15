#include "image_stream.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "camera.h"

#define JPEG_MAX_ATTEMPTS   4
#define PSRAM_BUFFER_COUNT   5
#define PSRAM_BUFFER_SIZE    (300 * 1024)


typedef enum
{
    PSRAM_BUF_EMPTY = 0,
    PSRAM_BUF_FILLING,
    PSRAM_BUF_FULL,
    PSRAM_BUF_PARTIAL

} psram_buf_state_t;


typedef struct
{
    uint8_t *data;
    size_t len;
    psram_buf_state_t state;

} psram_stream_buffer_t;


/* =========================================================
 * PSRAM buffers
 * ========================================================= */

static psram_stream_buffer_t
    psram_buf[PSRAM_BUFFER_COUNT];


/*
 * Current buffer camera is writing into.
 */
static int current_buffer = 0;


/*
 * JPEG detection
 */
static bool jpeg_started = false;
static bool jpeg_finished = false;
static bool jpeg_incomplete = false;
static int jpeg_attempt = 0;

/*
 * Used for detecting FF D8 across DMA boundaries.
 */
static uint8_t previous_input_byte = 0;


/*
 * Used for detecting FF D9.
 */
static uint8_t previous_jpeg_byte = 0;


/*
 * Total JPEG bytes stored.
 */
static size_t total_jpeg_bytes = 0;


/*
 * True if all 5 buffers become full before JPEG finishes.
 */
static bool psram_overflow = false;

static TaskHandle_t jpeg_parser_task_handle = NULL;

/*
 * Absolute number of bytes written by camera.
 */
static volatile size_t stream_write_position = 0;

/*
 * Absolute number of bytes already inspected by parser.
 */
static size_t parser_position = 0;

static uint8_t parser_previous_byte = 0;
static bool parser_have_previous = false;

static bool parser_jpeg_started = false;
static int parser_attempt = 0;

static size_t jpeg_start_position = 0;
static size_t jpeg_end_position = 0;


static const char *TAG = "IMAGE_STREAM";

#if 0
/* =========================================================
 * PRINT FINAL REPORT
 * ========================================================= */

static void print_psram_report(void)
{
    int used_buffers = 0;
    int full_buffers = 0;
    int empty_buffers = 0;

    ESP_LOGI(TAG,
             "========================================");

    ESP_LOGI(TAG,
             "PSRAM CAPTURE REPORT");

    ESP_LOGI(TAG,
             "Configured buffers : %d",
             PSRAM_BUFFER_COUNT);

    ESP_LOGI(TAG,
             "Each buffer        : %u bytes (%u KB)",
             (unsigned)PSRAM_BUFFER_SIZE,
             (unsigned)(PSRAM_BUFFER_SIZE / 1024));

    ESP_LOGI(TAG,
             "Total capacity     : %u bytes (%u KB)",
             (unsigned)(
                 PSRAM_BUFFER_COUNT *
                 PSRAM_BUFFER_SIZE
             ),
             (unsigned)(
                 (PSRAM_BUFFER_COUNT *
                  PSRAM_BUFFER_SIZE) / 1024
             ));


    for (int i = 0;
         i < PSRAM_BUFFER_COUNT;
         i++)
    {
        size_t len = psram_buf[i].len;

        if (len == 0)
        {
            empty_buffers++;

            ESP_LOGI(TAG,
                     "Buffer %d : EMPTY   0 / %u bytes",
                     i + 1,
                     (unsigned)PSRAM_BUFFER_SIZE);
        }

        else if (len == PSRAM_BUFFER_SIZE)
        {
            used_buffers++;
            full_buffers++;

            ESP_LOGI(TAG,
                     "Buffer %d : FULL    %u / %u bytes (100%%)",
                     i + 1,
                     (unsigned)len,
                     (unsigned)PSRAM_BUFFER_SIZE);
        }

        else
        {
            used_buffers++;

            unsigned percent =
                (unsigned)(
                    (len * 100U) /
                    PSRAM_BUFFER_SIZE
                );

            ESP_LOGI(TAG,
                     "Buffer %d : PARTIAL %u / %u bytes (%u%%)",
                     i + 1,
                     (unsigned)len,
                     (unsigned)PSRAM_BUFFER_SIZE,
                     percent);
        }
    }


    ESP_LOGI(TAG,
             "----------------------------------------");

    ESP_LOGI(TAG,
             "Buffers used  : %d / %d",
             used_buffers,
             PSRAM_BUFFER_COUNT);

    ESP_LOGI(TAG,
             "Buffers full  : %d",
             full_buffers);

    ESP_LOGI(TAG,
             "Buffers empty : %d",
             empty_buffers);

    ESP_LOGI(TAG,"Raw stream bytes stored : %u",(unsigned)stream_write_position);

    if (psram_overflow)
    {
        ESP_LOGE(TAG,
                 "RESULT: PSRAM BUFFER OVERFLOW");
    }
    else
    {
        ESP_LOGI(TAG,
                 "RESULT: JPEG FIT INSIDE SEGMENTED PSRAM");
    }

    ESP_LOGI(TAG,
             "========================================");
}
#endif

/* =========================================================
 * WRITE ONE BYTE INTO SEGMENTED PSRAM
 * ========================================================= */



static bool psram_write_block(
    const uint8_t *data,
    size_t len)
{
    size_t offset = 0;

    while (offset < len)
    {
        if (current_buffer >= PSRAM_BUFFER_COUNT)
        {
            psram_overflow = true;

            ESP_LOGE(TAG,
                     "ALL PSRAM BUFFERS ARE FULL");

            return false;
        }

        psram_stream_buffer_t *buf =
            &psram_buf[current_buffer];

        if (buf->state == PSRAM_BUF_EMPTY)
        {
            buf->state = PSRAM_BUF_FILLING;

            ESP_LOGI(TAG,
                     "Camera started writing Buffer %d",
                     current_buffer + 1);
        }

        size_t free_space =
            PSRAM_BUFFER_SIZE - buf->len;

        size_t remaining =
            len - offset;

        size_t copy_len =
            (remaining < free_space)
            ? remaining
            : free_space;

        memcpy(
            buf->data + buf->len,
            data + offset,
            copy_len
        );

        buf->len += copy_len;

        offset += copy_len;

        stream_write_position += copy_len;

        if (buf->len == PSRAM_BUFFER_SIZE)
        {
            buf->state = PSRAM_BUF_FULL;

            ESP_LOGI(TAG,
                     "Buffer %d FULL: %u bytes",
                     current_buffer + 1,
                     (unsigned)buf->len);

            current_buffer++;

            if (current_buffer < PSRAM_BUFFER_COUNT)
            {
                ESP_LOGI(TAG,
                         "Switching to Buffer %d",
                         current_buffer + 1);
            }
        }
    }

    return true;
}

static bool psram_read_absolute(
    size_t position,
    uint8_t *value)
{
    int buffer_index =
        position / PSRAM_BUFFER_SIZE;

    size_t offset =
        position % PSRAM_BUFFER_SIZE;

    if (buffer_index >= PSRAM_BUFFER_COUNT)
    {
        return false;
    }

    if (offset >= psram_buf[buffer_index].len)
    {
        return false;
    }

    *value =
        psram_buf[buffer_index].data[offset];

    return true;
}


static void jpeg_parser_task(void *arg)
{
    ESP_LOGI(TAG,
             "JPEG parser task started");

    while (1)
    {
        /*
         * Sleep until camera writes more data.
         */
        ulTaskNotifyTake(
            pdTRUE,
            portMAX_DELAY
        );


        /*
         * Take a snapshot.
         *
         * Camera may continue writing after this,
         * but parser only scans data that was already committed.
         */
        size_t available =
            stream_write_position;


        while (parser_position < available)
        {
            uint8_t b = 0;

            if (!psram_read_absolute(
                    parser_position,
                    &b))
            {
                break;
            }


            if (parser_have_previous)
            {
                /* ==============================
                 * FF D8 = JPEG start
                 * ============================== */
                if (parser_previous_byte == 0xFF &&
                    b == 0xD8)
                {
                    if (!parser_jpeg_started)
                    {
                        parser_jpeg_started = true;

                        parser_attempt = 1;

                        jpeg_start_position =
                            parser_position - 1;

                        ESP_LOGI(TAG,
                                 "JPEG SOI FOUND at absolute byte %u",
                                 (unsigned)jpeg_start_position);

                        ESP_LOGI(TAG,
                                 "JPEG ATTEMPT 1 / %d",
                                 JPEG_MAX_ATTEMPTS);
                    }
                    else
                    {
                        parser_attempt++;

                        ESP_LOGW(TAG,
                                 "SECOND FF D8 FOUND BEFORE FF D9");

                        ESP_LOGW(TAG,
                                 "JPEG ATTEMPT %d STARTED",
                                 parser_attempt);


                        /*
                         * New FF D8 becomes new JPEG start.
                         *
                         * IMPORTANT:
                         * do NOT reset PSRAM here.
                         */
                        jpeg_start_position =
                            parser_position - 1;


                        if (parser_attempt >
                            JPEG_MAX_ATTEMPTS)
                        {
                            jpeg_incomplete = true;

                            ESP_LOGE(TAG,
                                     "PHOTO WAS NOT COMPLETED");

                            ESP_LOGE(TAG,
                                     "MAX JPEG ATTEMPTS REACHED");

                            //camera_stop_stream_capture();

                            break;
                        }
                    }
                }


                /* ==============================
                 * FF D9 = JPEG end
                 * ============================== */
                if (parser_jpeg_started &&
                    parser_previous_byte == 0xFF &&
                    b == 0xD9)
                {
                    jpeg_end_position =
                        parser_position;

                    jpeg_finished = true;

                    size_t jpeg_size =
                        jpeg_end_position -jpeg_start_position +1;

                    ESP_LOGI(TAG,
                            "JPEG EOI FOUND: FF D9");

                    ESP_LOGI(TAG,
                            "PHOTO COMPLETE");

                    ESP_LOGI(TAG,
                            "JPEG START POSITION = %u",
                            (unsigned)jpeg_start_position);

                    ESP_LOGI(TAG,
                            "JPEG END POSITION = %u",
                            (unsigned)jpeg_end_position);

                    ESP_LOGI(TAG,
                            "JPEG SIZE = %u bytes",
                            (unsigned)jpeg_size);

                    /*
                    * Count the D9 byte as processed.
                    */
                    parser_position++;

                    //camera_stop_stream_capture();

                    break;
                }
            }


            parser_previous_byte = b;
            parser_have_previous = true;

            parser_position++;
        }
    }
}
/* =========================================================
 * INITIALIZE 5 x 300 KB
 * ========================================================= */

esp_err_t image_stream_init(void)
{
    ESP_LOGI(TAG,
             "========================================");

    ESP_LOGI(TAG,
             "INITIALIZING SEGMENTED PSRAM");

    ESP_LOGI(TAG,
             "Free PSRAM before allocation: %u bytes",
             (unsigned)
             heap_caps_get_free_size(
                 MALLOC_CAP_SPIRAM
             ));


    for (int i = 0;
         i < PSRAM_BUFFER_COUNT;
         i++)
    {
        psram_buf[i].data =
            heap_caps_malloc(
                PSRAM_BUFFER_SIZE,
                MALLOC_CAP_SPIRAM |
                MALLOC_CAP_8BIT
            );


        if (psram_buf[i].data == NULL)
        {
            ESP_LOGE(TAG,
                     "Buffer %d allocation FAILED",
                     i + 1);

            return ESP_ERR_NO_MEM;
        }


        psram_buf[i].len = 0;
        psram_buf[i].state =
            PSRAM_BUF_EMPTY;


        ESP_LOGI(TAG,
                 "Buffer %d allocated: %u bytes @ %p",
                 i + 1,
                 (unsigned)PSRAM_BUFFER_SIZE,
                 psram_buf[i].data);
    }


    current_buffer = 0;

    jpeg_started = false;
    jpeg_finished = false;
    jpeg_incomplete = false;

    jpeg_attempt = 0;
    previous_input_byte = 0;
    previous_jpeg_byte = 0;

    total_jpeg_bytes = 0;

    psram_overflow = false;


    ESP_LOGI(TAG,
             "----------------------------------------");

    ESP_LOGI(TAG,
             "Buffer count   : %d",
             PSRAM_BUFFER_COUNT);

    ESP_LOGI(TAG,
             "Buffer size    : %u KB",
             (unsigned)(
                 PSRAM_BUFFER_SIZE / 1024
             ));

    ESP_LOGI(TAG,
             "Total capacity : %u KB",
             (unsigned)(
                 (PSRAM_BUFFER_COUNT *
                  PSRAM_BUFFER_SIZE) / 1024
             ));

    ESP_LOGI(TAG,
             "Free PSRAM after allocation: %u bytes",
             (unsigned)
             heap_caps_get_free_size(
                 MALLOC_CAP_SPIRAM
             ));

    ESP_LOGI(TAG,
             "SEGMENTED PSRAM READY");

    ESP_LOGI(TAG,
             "========================================");

    stream_write_position = 0;
    parser_position = 0;

    parser_previous_byte = 0;
    parser_have_previous = false;

    parser_jpeg_started = false;
    parser_attempt = 0;

    jpeg_start_position = 0;
    jpeg_end_position = 0;

    BaseType_t task_result =
    xTaskCreate(
        jpeg_parser_task,
        "jpeg_parser",
        4096,
        NULL,
        10,
        &jpeg_parser_task_handle
    );

    if (task_result != pdPASS)
    {
        ESP_LOGE(TAG,"Failed to create JPEG parser task");

    return ESP_FAIL;
}

ESP_LOGI(TAG,
         "JPEG parser task created");
    return ESP_OK;
}



/* =========================================================
 * CAMERA DMA CALLBACK
 * ========================================================= */

size_t image_stream_push(
    const uint8_t *data,
    size_t len)
{
    if (data == NULL || len == 0)
    {
        return 0;
    }

    if (jpeg_finished ||
        jpeg_incomplete ||
        psram_overflow)
    {
        return len;
    }


    /*
     * PRIORITY #1:
     *
     * Copy incoming DMA data immediately.
     *
     * NO FF D8 search.
     * NO FF D9 search.
     * NO JPEG parsing.
     */
    if (!psram_write_block(
            data,
            len))
    {
        psram_overflow = true;

        ESP_LOGE(TAG,
                 "PSRAM OVERFLOW");

        //camera_stop_stream_capture();

        return 0;
    }


    /*
     * Tell parser that more data exists.
     *
     * This is very fast.
     */
    if (jpeg_parser_task_handle != NULL)
    {
        xTaskNotifyGive(
            jpeg_parser_task_handle
        );
    }


    return len;
}


/* =========================================================
 * STATUS
 * ========================================================= */



bool image_stream_is_finished(void)
{
    return jpeg_finished ||
           jpeg_incomplete ||
           psram_overflow;
}