#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "interval_task.h"
#include "timer.h"

typedef struct
{
    uint32_t last_update;
    uint32_t last_publish;
} interval_task_state_t;

void task(void *pvparameters)
{
    uint32_t now = 0;
    get_current_time(&now);

    interval_task_state_t task_state = {
        .last_publish = now,
        .last_update = now};

    uint32_t last_update_interval = 0, last_publish_interval = 0;

    interval_task_interface_t *task_interface = (interval_task_interface_t *)pvparameters;

    if (task_interface->init)
    {
        task_interface->init();
    }

    while (1)
    {
        if (task_interface->start)
        {
            task_interface->start();
        }

        get_current_time(&now);

        last_update_interval = (now - task_state.last_update);
        last_publish_interval = (now - task_state.last_publish);

        ESP_LOGD(task_interface->name, "Now: %lu, Force: %u, Last Update: %lu, Last Pub: %lu", now, task_interface->force_publish, last_update_interval, last_publish_interval);

        if ((task_interface->force_publish > 0) || (last_update_interval >= task_interface->update_interval))
        {
            task_state.last_update = now;

            if (task_interface->disable_update == false)
            {
                ESP_LOGD(task_interface->name, "Update");

                if (task_interface->update)
                {
                    task_interface->update();
                }
            }
        }

        if ((task_interface->force_publish > 0) || (last_publish_interval >= task_interface->publish_interval))
        {
            task_state.last_publish = now;

            if (task_interface->disable_publish == false)
            {
                ESP_LOGD(task_interface->name, "Publish");

                if (task_interface->force_publish > 0)
                {
                    task_interface->force_publish -= 1;
                }

                if (task_interface->publish)
                {
                    task_interface->publish();
                }
            }
        }

        if (task_interface->end)
        {
            task_interface->end();
        }

        vTaskDelay(pdMS_TO_TICKS(task_interface->task_interval));
    }
}
