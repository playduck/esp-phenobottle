#include "timer.h"

esp_err_t get_current_time(uint32_t *time) {
    struct timeval tv;
    esp_err_t err = ESP_OK;

    err = gettimeofday(&tv, NULL);
    if (err != ESP_OK) {
        return err;
    }

    *time = (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
    return err;
}
