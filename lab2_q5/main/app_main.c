#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#ifndef LED_PIN
#define LED_PIN 2
#endif

static const char *TAG = "lab2_msg";

/* Priorities: use SAME or DISTINCT per your experiment */
static const int PRIO_TASK1_LED_ON   = 3;  /* highest (case a); set 2 for same-priority test */
static const int PRIO_TASK2_LED_OFF  = 2;
static const int PRIO_TASK3_STATUS   = 1;

/* ---------- Message queue for LED commands ---------- */
typedef enum {
    LED_CMD_ON  = 1,
    LED_CMD_OFF = 2,
} led_cmd_t;

/* Single-slot queue (+ xQueueOverwrite) = latest command wins */
static QueueHandle_t g_ledQ;

/* --- GPIO helpers --- */
static void led_init(void) {
    gpio_config_t io = {0};
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = 1ULL << LED_PIN;
    gpio_config(&io);
    gpio_set_level(LED_PIN, 0); /* start OFF */
}
static inline void led_on(void)  { gpio_set_level(LED_PIN, 1); }
static inline void led_off(void) { gpio_set_level(LED_PIN, 0); }

/* -------- LED driver: the ONLY task that touches GPIO --------
   Receives commands and sets the pin. This serializes access and
   eliminates any need for mutex/PI/semaphores on the LED itself. */
static void task_led_driver(void *arg) {
    (void)arg;
    led_cmd_t cmd;
    ESP_LOGI(TAG, "LED driver started (queue len=1)");

    for (;;) {
        if (xQueueReceive(g_ledQ, &cmd, portMAX_DELAY) == pdTRUE) {
            if (cmd == LED_CMD_ON) {
                led_on();
                ESP_LOGI(TAG, "DRV: LED ON");
            } else if (cmd == LED_CMD_OFF) {
                led_off();
                ESP_LOGI(TAG, "DRV: LED OFF");
            }
        }
    }
}

/* T1: send ON, actively wait 0.5 s, yield, then block 1 tick (so lower prios run) */
static void task_led_on_sender(void *arg) {
    (void)arg;
    const TickType_t half_sec = pdMS_TO_TICKS(500);

    for (;;) {
        led_cmd_t cmd = LED_CMD_ON;
        xQueueOverwrite(g_ledQ, &cmd);  /* non-blocking, latest-wins */
        ESP_LOGI(TAG, "T1: sent LED_CMD_ON, busy-wait 500 ms");

        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < half_sec) { /* spin */ }

        taskYIELD();
        vTaskDelay(1);
    }
}

/* T2: send OFF, then delay 1 s */
static void task_led_off_sender(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);

    for (;;) {
        led_cmd_t cmd = LED_CMD_OFF;
        xQueueOverwrite(g_ledQ, &cmd);  /* non-blocking, latest-wins */
        ESP_LOGI(TAG, "T2: sent LED_CMD_OFF (delay 1000 ms)");
        vTaskDelay(one_sec);
    }
}

/* T3: status every 1 s */
static void task_status(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);
    for (;;) {
        ESP_LOGI(TAG, "T3: tick=%lu", (unsigned long)xTaskGetTickCount());
        vTaskDelay(one_sec);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "app_main: init (message queue)");
    led_init();

    /* Single-slot command queue; xQueueOverwrite() requires length 1 */
    g_ledQ = xQueueCreate(1, sizeof(led_cmd_t));
    configASSERT(g_ledQ != NULL);

    /* Start tasks */
    xTaskCreate(task_led_driver,     "tLED_DRV",  1024, NULL, PRIO_TASK2_LED_OFF+1, NULL);
    xTaskCreate(task_led_on_sender,  "tLED_ON",   1024, NULL, PRIO_TASK1_LED_ON,    NULL);
    xTaskCreate(task_led_off_sender, "tLED_OFF",  1024, NULL, PRIO_TASK2_LED_OFF,   NULL);
    xTaskCreate(task_status,         "tSTATUS",   1024, NULL, PRIO_TASK3_STATUS,    NULL);
}
