#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include <cJSON.h>

#include "interval_task.h"
#include "client.h"
#include "task_manager.h"

#include "endpoints.h"

extern interval_task_interface_t camera_task_interface;

static const char *TAG = "Task Manager";
uint8_t t = 0;

char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

static esp_err_t parseState()
{
    char *jsonString = NULL;
    char *headerEnd = NULL;
    char *jsonStart = NULL;

    cJSON *root = NULL;

    esp_err_t ret_val = ESP_FAIL;

    root = cJSON_ParseWithLength(local_response_buffer, MAX_HTTP_OUTPUT_BUFFER + 1);
    if (!root)
    {
        ESP_LOGE(TAG, "Error parsing JSON\n");
        goto end;
    }

    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (!state)
    {
        ESP_LOGE(TAG, "Error: 'state' not found\n");
        goto end;
    }

    printf("State: %s\n", cJSON_Print(state));

    cJSON *tasks = cJSON_GetObjectItem(root, "tasks");
    if (!tasks)
    {
        ESP_LOGE(TAG, "Error: 'tasks' not found\n");
        goto end;
    }

    cJSON *task;
    int task_count = cJSON_GetArraySize(tasks);
    for (int i = 0; i < task_count; i++)
    {
        task = cJSON_GetArrayItem(tasks, i);

        cJSON *task_id = cJSON_GetObjectItem(task, "task_id");
        cJSON *device_id = cJSON_GetObjectItem(task, "device_id");
        cJSON *task_name = cJSON_GetObjectItem(task, "task_name");
        cJSON *task_type = cJSON_GetObjectItem(task, "task_type");
        cJSON *task_start = cJSON_GetObjectItem(task, "task_start");
        cJSON *task_duration = cJSON_GetObjectItem(task, "task_duration");

        printf("Task:\n");
        printf("  task_id: %d\n", task_id->valueint);
        printf("  device_id: %d\n", device_id->valueint);
        printf("  task_name: %s\n", task_name->valuestring);
        printf("  task_type: %s\n", task_type->valuestring);
        printf("  task_start: %s\n", task_start->valuestring);
        printf("  task_duration: %s\n", task_duration->valuestring);
        cJSON *task_period = cJSON_GetObjectItem(task, "task_period");
        if (cJSON_IsNull(task_period))
        {
            printf("  task_period: null\n");
        }
        else
        {
            printf("  task_period: %s\n", task_period->valuestring);
        }

        printf("\n");
    }

    cJSON *settings = cJSON_GetObjectItem(root, "settings");
    if (settings)
    {
        printf("Settings: %s\n", cJSON_Print(settings));
    }

    cJSON *actions = cJSON_GetObjectItem(root, "actions");
    if (actions)
    {
        printf("Actions: %s\n", cJSON_Print(actions));
    }
    ret_val = ESP_OK;

end:
    if (root)
    {
        cJSON_Delete(root);
    }
    free(jsonString);
    return ret_val;
}

esp_err_t task_manager_init()
{
    return ESP_OK;
}

esp_err_t task_manager_start()
{
    return ESP_OK;
}

esp_err_t task_manager_update()
{
    esp_http_client_config_t *config = get_config();
    config->url = API_V1_GET_STATE;
    config->user_data = local_response_buffer;

    esp_http_client_handle_t client = esp_http_client_init(config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));

        if(parseState() != ESP_OK)  {
            ESP_LOGE(TAG, "Failed to parse json state: \n\"%s\"", local_response_buffer);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    release_config();

    return err;
}

esp_err_t task_manager_publish()
{
    return ESP_OK;
}

esp_err_t task_manager_end()
{
    return ESP_OK;
}
