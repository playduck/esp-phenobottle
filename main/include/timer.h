#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <esp_err.h>

esp_err_t get_current_time(uint32_t *time);

#endif
