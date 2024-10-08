#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_pm.h"
#include "esp_camera.h"

#include "timer.h"
#include "client.h"
#include "sensors/camera.h"
#include "interval_task.h"
#include "http_status_codes.h"
#include "endpoints.h"

static const char *TAG = "CAM";

/* HTTP multipart form bounadry, refer https://www.ietf.org/rfc/rfc2046.txt */
#define FILE_BOUNDARY "WarrSpacelabsDataBoundary"

/* temporary assembly and response buffer */
#define TMP_BUFFER_LENGTH (1024)
#define TMP_HEAD_LENGTH (512)
#define TMP_END_LENGTH (128)

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

    .xclk_freq_hz = 20000000, // EXPERIMENTAL: Set..16MHz on ESP32-S2 or ESP32-S3..enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,   // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 4,                // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,                     // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM, // Enable PSRAM support in menuconfig !IMPORTANT
    .grab_mode = CAMERA_GRAB_LATEST    // CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

static uint32_t timestamp = 0;
static camera_fb_t *fb = NULL;
static esp_pm_lock_handle_t cam_power_lock;

camera_fb_t *take_image()
{
    // turn on flash and wait for AGC..settle
    gpio_set_level(CAM_PIN_FLASH, 1);
    vTaskDelay(pdMS_TO_TICKS(800)); // TODO figure out minimum AGC flash time

    esp_pm_lock_acquire(cam_power_lock);

    // acquire a frame
    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);

    fb = esp_camera_fb_get();
    get_current_time(&timestamp);

    if (!fb)
    {
        ESP_LOGE(TAG, "Camera Capture Failed");
        // blink an error pattern
        for (uint8_t i = 0; i < 6; i++)
        {
            gpio_set_level(CAM_PIN_FLASH, 1);
            vTaskDelay(pdMS_TO_TICKS(10));
            gpio_set_level(CAM_PIN_FLASH, 0);
            vTaskDelay(pdMS_TO_TICKS(70));
        }
    }
    else
    {
        ESP_LOGI(TAG, "Frame Captured");

        // blink flash..show an image was taken
        for (uint8_t i = 0; i < 3; i++)
        {
            gpio_set_level(CAM_PIN_FLASH, 1);
            vTaskDelay(pdMS_TO_TICKS(20));
            gpio_set_level(CAM_PIN_FLASH, 0);
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }

    esp_pm_lock_release(cam_power_lock);
    return fb;
}

esp_err_t post_frame(camera_fb_t *fb, uint32_t timestamp)
{
    esp_err_t ret = ESP_OK;
    if (!fb)
    {
        ESP_LOGE(TAG, "Cannot publish empty image");
        return ret;
    }

    char tmp_buf[TMP_BUFFER_LENGTH];
    char HEAD[TMP_HEAD_LENGTH];
    char END[TMP_END_LENGTH];

    // setup client connection (wait for other processses..finish)
    esp_http_client_config_t *config = get_config();
    config->url = API_V1_POST_IMAGE;

    esp_http_client_handle_t client = esp_http_client_init(config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);

    // assemble request headers
    sprintf(tmp_buf, "%d", 1);
    esp_http_client_set_header(client, "Device-Id", tmp_buf);
    sprintf(tmp_buf, "%lu", timestamp);
    esp_http_client_set_header(client, "Timestamp", tmp_buf);
    esp_http_client_set_header(client, "Form-Mime", "image/jpeg");
    sprintf(tmp_buf, "multipart/form-data; boundary=%s", FILE_BOUNDARY);
    esp_http_client_set_header(client, "Content-Type", tmp_buf);

    // assemble multipart body head
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "--%s\r\n", FILE_BOUNDARY);
    strcpy(HEAD, tmp_buf);
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n");
    strcat(HEAD, tmp_buf);
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "Content-Type: application/octet-stream\r\n\r\n");
    strcat(HEAD, tmp_buf);

    // assemble multipart body end
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "\r\n--%s--\r\n\r\n", FILE_BOUNDARY);
    strcpy(END, tmp_buf);

    // write total length of body into the header
    uint32_t total_len_to_send = fb->len + strnlen(HEAD, TMP_HEAD_LENGTH) + strnlen(END, TMP_END_LENGTH);
    ESP_LOGI(TAG, "total length: %lu", total_len_to_send);
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "%lu", total_len_to_send);
    esp_http_client_set_header(client, "Content-Length", tmp_buf);

    // open the connection
    ESP_LOGI(TAG, "Opening Connection");
    esp_http_client_open(client, total_len_to_send);

    // write the HEAD header
    esp_http_client_write(client, HEAD, strnlen(HEAD, TMP_HEAD_LENGTH));

    // prepare..send the framebuffer
    uint8_t *buffer = fb->buf;
    uint8_t send_buffer[MAX_HTTP_OUTPUT_BUFFER];
    size_t remaining_bytes = fb->len;

    // send fb in chunks
    while (remaining_bytes > 0)
    {
        // calculate the number of bytes..send in this iteration
        size_t bytes_to_send = (remaining_bytes < MAX_HTTP_OUTPUT_BUFFER) ? remaining_bytes : MAX_HTTP_OUTPUT_BUFFER;

        // copy data from 'buffer'..'send_buffer'
        memcpy(send_buffer, buffer, bytes_to_send);
        buffer += bytes_to_send;
        remaining_bytes -= bytes_to_send;

        // send the incremental package
        esp_http_client_write(client, (char *)send_buffer, bytes_to_send);

        // zero out buffer for next iteration
        bzero(send_buffer, MAX_HTTP_OUTPUT_BUFFER);
    }

    // send multipart end
    esp_http_client_write(client, END, strnlen(END, TMP_END_LENGTH));

    // read server response
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= TMP_BUFFER_LENGTH)
    {
        int read_len = esp_http_client_read(client, tmp_buf, content_length);
        if (read_len <= 0)
        {
            ESP_LOGE(TAG, "Error read data");
            ret = ESP_FAIL;
        }
        ESP_LOGI(TAG, "read data: %s", tmp_buf);
        ESP_LOGI(TAG, "read_len = %d", read_len);
    }
    else
    {
        ESP_LOGE(TAG, "Response too long for buffer (%d of %d)", content_length, TMP_BUFFER_LENGTH);
    }

    // get the response status code
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status = %d, content_length = %llu",
             status_code,
             esp_http_client_get_content_length(client));

    // finalize the connection no matter what happend
    esp_http_client_close(client);

    if (status_code >= HTTP_STATUS_BAD_REQUEST)
    {
        ESP_LOGE(TAG, "Error sending file!");
        ret = ESP_FAIL;
    }

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(ret));
    }

    // cleanup
    esp_http_client_cleanup(client);
    release_config();

    return ret;
}

esp_err_t camera_init()
{
    esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "Cam Lock", &cam_power_lock);
    esp_pm_lock_acquire(cam_power_lock);

    gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
    gpio_set_level(CAM_PIN_PWDN, 1);

    gpio_set_direction(CAM_PIN_FLASH, GPIO_MODE_OUTPUT);
    for (uint8_t i = 0; i < 6; i++)
    {
        gpio_set_level(CAM_PIN_FLASH, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);
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

    // Warm-up loop..discard first few frames
    ESP_LOGI(TAG, "Warming up camera...");

    // OV2640 Reg map: https://www.plexishop.it/pdf/OV2640.pdf
    sensor_t *s = esp_camera_sensor_get();
    s->set_brightness(s, 1);                // -2..2, internally 0..5
    s->set_contrast(s, -1);                  // -2..2, internally 0..5
    s->set_saturation(s, 1);                // -2..2, internally 0..5
    s->set_special_effect(s, 0);            // 0..6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);                  // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);                  // 0 = disable , 1 = enable
    s->set_wb_mode(s, 1);                   // 0..4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 1);             // 0 = disable , 1 = enable
    s->set_aec2(s, 1);                      // 0 = disable , 1 = enable
    s->set_ae_level(s, 1);                  // -2..2
    s->set_aec_value(s, 300);               // 0..1200
    s->set_gain_ctrl(s, 1);                 // 0 = disable , 1 = enable
    s->set_agc_gain(s, 16);                  // 0..30
    s->set_gainceiling(s, GAINCEILING_64X); // 0..6
    s->set_bpc(s, 1);                       // black pixel correction 0 = disable , 1 = enable
    s->set_wpc(s, 1);                       // white pixel correction 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);                   // 0 = disable , 1 = enable
    s->set_lenc(s, 0);                      // lens correction 0 = disable , 1 = enable
    s->set_hmirror(s, 0);                   // horizontal mirror 0 = disable , 1 = enable
    s->set_vflip(s, 0);                     // vertical flip 0 = disable , 1 = enable
    s->set_dcw(s, 0);                       // downsize? 0 = disable , 1 = enable
    s->set_colorbar(s, 0);                  // test pattern 0 = disable , 1 = enable

    // take a few images and discard them
    // this wakes the sensor up and initilizes AWB
    for (int i = 0; i < 10; i++)
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            continue;
        }
        esp_camera_fb_return(fb);
    }

    ESP_LOGI(TAG, "Camera done");
    esp_pm_lock_release(cam_power_lock);
    return ESP_OK;
}

esp_err_t camera_start()
{
    return ESP_OK;
}

esp_err_t camera_update()
{
    fb = take_image();
    return ESP_OK;
}

esp_err_t camera_publish()
{
    if (!fb)
    {
        fb = take_image();
    }
    post_frame(fb, timestamp);
    return ESP_OK;
}

esp_err_t camera_end()
{
    if (fb)
    {
        esp_camera_fb_return(fb);
        fb = NULL;
    }
    return ESP_OK;
}
