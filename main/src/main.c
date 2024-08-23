#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_psram.h"

#include "sdkconfig.h"

#include "interval_task.h"
#include "time_sync.h"
#include "client.h"
#include "tasks.h"
#include "measurement.h"

static const char *TAG = "MAIN";

/* NVS fetch and store time period: hours * minutes * seconds * milliseconds * microseconds */
#define NVS_TIME_PERIOD_US (6ULL * 60ULL * 60ULL * 1000ULL * 1000ULL)

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Hello World, string app_main");

    // FIXME do something less dumb here
    ESP_ERROR_CHECK(example_connect());

    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        ESP_LOGI(TAG, "Updating time from NVS");
        ESP_ERROR_CHECK(update_time_from_nvs());
    }

    // get time from SNTP periodically
    const esp_timer_create_args_t nvs_update_timer_args = {
        .callback = (void *)&fetch_and_store_time_in_nvs,
    };
    esp_timer_handle_t nvs_update_timer;
    ESP_ERROR_CHECK(esp_timer_create(&nvs_update_timer_args, &nvs_update_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(nvs_update_timer, NVS_TIME_PERIOD_US));

    ESP_ERROR_CHECK(client_init());

    xTaskCreate(&send_measurement_task, "Measurement", 4096, (void*)NULL, configMAX_PRIORITIES - 4, NULL);

    xTaskCreate(&task, task_manager_interface.name, 4096, (void*)&task_manager_interface, configMAX_PRIORITIES - 3, NULL);
    xTaskCreate(&task, camera_task_interface.name, 8192, (void*)&camera_task_interface, configMAX_PRIORITIES - 5, NULL);
    xTaskCreate(&task, temp_task_interface.name, 2048, (void*)&temp_task_interface, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(&task, gas_task_interface.name, 2048, (void*)&gas_task_interface, configMAX_PRIORITIES - 6, NULL);
    xTaskCreate(&task, od_task_interface.name, 2048, (void*)&od_task_interface, configMAX_PRIORITIES - 4, NULL);

    xTaskCreate(&task, illumination_task_interface.name, 2048, (void*)&illumination_task_interface, configMAX_PRIORITIES - 8, NULL);
    xTaskCreate(&task, mixing_task_interface.name, 2048, (void*)&mixing_task_interface, configMAX_PRIORITIES - 5, NULL);
}
