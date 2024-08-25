#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#define STATS_DURATION pdMS_TO_TICKS(30000)
#define STATS_BLINDTIME pdMS_TO_TICKS(10000)
#define ARRAY_SIZE_OFFSET 5 // Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE

static const char *TAG = "STATS";

static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
{
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    configRUN_TIME_COUNTER_TYPE start_run_time, end_run_time;
    esp_err_t ret;

    // Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    // Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0)
    {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    // Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    // Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0)
    {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    // Calculate total_elapsed_time in units of run time stats clock period.
    uint32_t total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0)
    {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }
    printf("| Task            | Run Time | Percentage\n");
    printf("| --------------- | -------- | ----------\n");
    // Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++)
    {
        int k = -1;
        for (int j = 0; j < end_array_size; j++)
        {
            if (start_array[i].xHandle == end_array[j].xHandle)
            {
                k = j;
                // Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }
        // Check if matching task found
        if (k >= 0)
        {
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * CONFIG_FREERTOS_NUMBER_OF_CORES);
            printf("| %-15s | %-8lu | %02lu%% [", start_array[i].pcTaskName, task_elapsed_time, percentage_time);

            const uint8_t bar_length = 50;
            for(uint8_t i = 0; i < bar_length; i++) {
                float percentage = ((i+1.0f) / (float)bar_length) * 100.0f;

                if(percentage_time >= percentage)   {
                    printf("*");
                }   else    {
                    printf(" ");
                }
            }
            printf("]\n");
        }
    }

    // Print unmatched tasks
    for (int i = 0; i < start_array_size; i++)
    {
        if (start_array[i].xHandle != NULL)
        {
            printf("| %-15s | Deleted\n", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++)
    {
        if (end_array[i].xHandle != NULL)
        {
            printf("| %-15s | Created\n", end_array[i].pcTaskName);
        }
    }
    ret = ESP_OK;

exit: // Common return path
    free(start_array);
    free(end_array);
    return ret;
}

void stats_task(void *arg)
{
    // Print real time stats periodically
    while (1)
    {
        ESP_LOGI(TAG, "\n\nGetting real time stats over %" PRIu32 " ticks\n", STATS_DURATION);
        if (print_real_time_stats(STATS_DURATION) != ESP_OK)
        {
            ESP_LOGE(TAG, "Error getting real time stats\n");
        }
        vTaskDelay(STATS_BLINDTIME);
    }
}
