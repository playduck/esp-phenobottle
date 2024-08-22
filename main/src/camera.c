#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#include "esp_camera.h"

static const char *TAG = "CAM";

extern esp_err_t _http_event_handler(esp_http_client_event_t *evt);
extern const char server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const char server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

#define CAM_PIN_FLASH 4

#define CAM_PIN_PWDN 32  // power down is not used
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000, // EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,   // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 40, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,      // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY // CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

esp_err_t camera_init()
{
    // power up the camera if PWDN pin is defined
    if (CAM_PIN_PWDN != -1)
    {
        gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
        gpio_set_level(CAM_PIN_PWDN, 1);

        // pinMode(CAM_PIN_PWDN, OUTPUT);
        // digitalWrite(CAM_PIN_PWDN, LOW);
    }

    gpio_set_direction(CAM_PIN_FLASH, GPIO_MODE_OUTPUT);
    for (uint8_t i = 0; i < 6; i++)
    {
        gpio_set_level(CAM_PIN_FLASH, 1);
        vTaskDelay(8 / portTICK_PERIOD_MS);
        gpio_set_level(CAM_PIN_FLASH, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    // Warm-up loop to discard first few frames
    ESP_LOGI(TAG, "Warming up camera...");
    for (int i = 0; i < 50; i++)
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            continue;
        }
        esp_camera_fb_return(fb);
    }
    ESP_LOGI(TAG, "Camera done...");

    return ESP_OK;
}

esp_err_t camera_capture()
{
    struct timeval tv;
    esp_err_t err = ESP_OK;

    gpio_set_level(CAM_PIN_FLASH, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // acquire a frame
    camera_fb_t *fb = esp_camera_fb_get();

    gettimeofday(&tv, NULL);
    uint32_t time = (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));

    // flash to show image was taken
    for (uint8_t i = 0; i < 3; i++)
    {
        gpio_set_level(CAM_PIN_FLASH, 1);
        vTaskDelay(20 / portTICK_PERIOD_MS);
        gpio_set_level(CAM_PIN_FLASH, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    if (!fb)
    {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    }
    // replace this with your own function
    //  process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);
    //  ESP_LOG_BUFFER_HEX_LEVEL(TAG, fb->buf, fb->len, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Frame Captured");

    char tmp_buf[256];

    esp_http_client_config_t config = {
        .url = "http://192.168.3.176:8080/api/v1/image",
        // .url = "https://warr.robin-prillwitz.de/api/v1/image",
        .event_handler = _http_event_handler,
        // .transport_type = HTTP_TRANSPORT_OVER_SSL,
        // .cert_pem = server_root_cert_pem_start,
        // .tls_version = ESP_HTTP_CLIENT_TLS_VER_TLS_1_2,
        // .username = CONFIG_USERNAME,
        // .password = CONFIG_PASSWORD,
        // .auth_type = HTTP_AUTH_TYPE_BASIC,
        // .max_authorization_retries = -1,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);

    sprintf(tmp_buf, "%d", 1);
    esp_http_client_set_header(client, "Device-Id", tmp_buf);
    sprintf(tmp_buf, "%lu", time);
    esp_http_client_set_header(client, "Timestamp", tmp_buf);
    esp_http_client_set_header(client, "Form-Mime", "image/jpeg");

#define FILE_BOUNDARY "WarrSpacelabsDataBoundary"

    sprintf(tmp_buf, "multipart/form-data; boundary=%s", FILE_BOUNDARY);
    esp_http_client_set_header(client, "Content-Type", tmp_buf);

    char BODY[512];
    char END[128];
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "--%s\r\n", FILE_BOUNDARY);
    strcpy(BODY, tmp_buf);
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n");
    strcat(BODY, tmp_buf);
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "Content-Type: application/octet-stream\r\n\r\n");
    strcat(BODY, tmp_buf);

    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "\r\n--%s--\r\n\r\n", FILE_BOUNDARY);
    strcpy(END, tmp_buf);

    memset(tmp_buf, 0, sizeof(tmp_buf));
    uint32_t total_len_to_send = fb->len + strlen(BODY) + strlen(END);
    sprintf(tmp_buf, "%lu", total_len_to_send);
    esp_http_client_set_header(client, "Content-Length", tmp_buf);

    ESP_LOGI(TAG, "total length: %lu", total_len_to_send);

    uint32_t bytes_sent = 0;

#define MAX_HTTP_OUTPUT_BUFFER (1024)

    esp_http_client_open(client, total_len_to_send);

    ESP_LOGI(TAG, "Opening Connection");
    esp_http_client_write(client, BODY, strnlen(BODY, 512));

    bytes_sent += strnlen(BODY, 512);

    uint8_t *buffer = fb->buf;
    size_t buffer_size = fb->len;

    uint8_t send_buffer[MAX_HTTP_OUTPUT_BUFFER];
    size_t remaining_bytes = buffer_size;

    while (remaining_bytes > 0)
    {
        // Calculate the number of bytes to send in this iteration
        size_t bytes_to_send = (remaining_bytes < MAX_HTTP_OUTPUT_BUFFER) ? remaining_bytes : MAX_HTTP_OUTPUT_BUFFER;

        // Copy data from 'buffer' to 'send_buffer'
        memcpy(send_buffer, buffer, bytes_to_send);
        buffer += bytes_to_send;
        remaining_bytes -= bytes_to_send;
        bytes_sent += bytes_to_send;
        esp_http_client_write(client, (char *)send_buffer, bytes_to_send);

        // ESP_LOGI(TAG, "Sending %d bytes (%d bytes remaining)", bytes_to_send, remaining_bytes);

        bzero(send_buffer, MAX_HTTP_OUTPUT_BUFFER);
    }

    esp_http_client_write(client, END, strnlen(END, 128));
    bytes_sent += strnlen(END, 128);

    ESP_LOGI(TAG, "Done writing, sent %lu bytes (of %lu expected)", bytes_sent, total_len_to_send);

    int content_length = esp_http_client_fetch_headers(client);
    int total_read_len = 0;
    if (total_read_len < content_length && content_length <= 1024)
    {
        int read_len = esp_http_client_read(client, tmp_buf, content_length);
        if (read_len <= 0)
        {
            ESP_LOGE(TAG, "Error read data");
            err = ESP_FAIL;
        }
        ESP_LOGI(TAG, "read data: %s", tmp_buf);
        ESP_LOGI(TAG, "read_len = %d", read_len);
    }

    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status = %d, content_length = %llu",
             status_code,
             esp_http_client_get_content_length(client));

    if (status_code >= 400)
    {
        ESP_LOGE(TAG, "Error sending file!");
        err = ESP_FAIL;
    }

    esp_http_client_close(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    // return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    vTaskDelete(NULL);
    return ESP_OK;
}
