#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#ifndef LED_PIN
#define LED_PIN 2
#endif

static const char *TAG = "lab2";

static const int PRIO_TASK1_LED_ON   = 3;
static const int PRIO_TASK2_LED_OFF  = 2;
static const int PRIO_TASK3_STATUS   = 1;

static SemaphoreHandle_t g_ledMutex;

static void led_init(void) {
    gpio_config_t io = {0};
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = 1ULL << LED_PIN;
    gpio_config(&io);
    gpio_set_level(LED_PIN, 0);
}
static inline void led_on(void)  { gpio_set_level(LED_PIN, 1); }
static inline void led_off(void) { gpio_set_level(LED_PIN, 0); }

/* ===== Alternation design =====
   - T1 (prio 3): every 2000 ms at phase 0, sets LED ON.
   - T2 (prio 2): every 2000 ms at phase +1000 ms, sets LED OFF.
   Result: ON 1 s, OFF 1 s, repeat.
*/

static void task_led_on_busywait(void *arg) {
    (void)arg;
    const TickType_t period2s = pdMS_TO_TICKS(2000);
    TickType_t next = xTaskGetTickCount();

    for (;;) {
        TickType_t req = xTaskGetTickCount();
        xSemaphoreTake(g_ledMutex, portMAX_DELAY);
        TickType_t got = xTaskGetTickCount();

        ESP_LOGI(TAG, "T1: took LED mutex (wait=%lu ms)",
                 (unsigned long)((got - req) * portTICK_PERIOD_MS));

        led_on();
        ESP_LOGI(TAG, "T1: LED ON (1s window)");

        xSemaphoreGive(g_ledMutex);
        vTaskDelayUntil(&next, period2s);
    }
}

static void task_led_off_delay(void *arg) {
    (void)arg;
    const TickType_t period2s = pdMS_TO_TICKS(2000);

    vTaskDelay(pdMS_TO_TICKS(1000));  /* phase offset +1s */
    TickType_t next = xTaskGetTickCount();

    for (;;) {
        TickType_t req = xTaskGetTickCount();
        xSemaphoreTake(g_ledMutex, portMAX_DELAY);
        TickType_t got = xTaskGetTickCount();

        ESP_LOGI(TAG, "T2: took LED mutex (wait=%lu ms)",
                 (unsigned long)((got - req) * portTICK_PERIOD_MS));

        led_off();
        ESP_LOGI(TAG, "T2: LED OFF (1s window)");

        xSemaphoreGive(g_ledMutex);
        vTaskDelayUntil(&next, period2s);
    }
}

static void task_status_uart(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);

    for (;;) {
        ESP_LOGI(TAG, "T3: status ticks=%lu",
                 (unsigned long)xTaskGetTickCount());
        vTaskDelay(one_sec);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "app_main: init");
    led_init();

    g_ledMutex = xSemaphoreCreateMutex();
    configASSERT(g_ledMutex != NULL);

    xTaskCreate(task_led_on_busywait, "tLED_ON",  1024, NULL, PRIO_TASK1_LED_ON,  NULL);
    xTaskCreate(task_led_off_delay,   "tLED_OFF", 1024, NULL, PRIO_TASK2_LED_OFF, NULL);
    xTaskCreate(task_status_uart,     "tUART",    1024, NULL, PRIO_TASK3_STATUS,  NULL);
}
