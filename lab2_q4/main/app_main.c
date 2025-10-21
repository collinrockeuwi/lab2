#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* ====== Switches for Q3 experiments ====== */
#define USE_MUTEX       1   /* 1 = case (c) PI enabled; 0 = case (b) no PI (binary semaphore) */
#define SAME_PRIORITY   0   /* 1 = all tasks same prio (run-to-completion), 0 = case (a) T1>T2>T3 */

/* ----- Hardware ----- */
#ifndef LED_PIN
#define LED_PIN 2
#endif

static const char *TAG = "lab2";

/* ----- Priorities ----- */
#if SAME_PRIORITY
  static const int PRIO_TASK1_LED_ON   = 2;
  static const int PRIO_TASK2_LED_OFF  = 2;
  static const int PRIO_TASK3_STATUS   = 2;
#else
  static const int PRIO_TASK1_LED_ON   = 3;  /* highest */
  static const int PRIO_TASK2_LED_OFF  = 2;
  static const int PRIO_TASK3_STATUS   = 1;
#endif

/* ----- LED lock: mutex (PI) or binary semaphore (no PI) ----- */
static SemaphoreHandle_t g_ledLock;
static const char *g_lockKind =
#if USE_MUTEX
  "mutex(PI)";
#else
  "binsem(noPI)";
#endif

/* ===== Tiny block-trace buffer (pairs with FreeRTOSConfig.h hooks) ===== */
typedef struct {
    uint32_t tick_ms;
    char     task[8];
    char     reason[16];
} BlockEvt;

#define TRACE_BUF_SZ 64
static volatile uint32_t trace_idx = 0;
static BlockEvt trace_buf[TRACE_BUF_SZ];

void app_trace_record(const char *reason) {
    /* Keep it tiny to minimize perturbation */
    uint32_t i = __atomic_fetch_add(&trace_idx, 1, __ATOMIC_RELAXED);
    BlockEvt *e = &trace_buf[i % TRACE_BUF_SZ];
    e->tick_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    snprintf(e->task,   sizeof(e->task),   "%s", pcTaskGetName(NULL));
    snprintf(e->reason, sizeof(e->reason), "%s", reason);
}

static void dump_trace_summary(void) {
    static uint32_t last_idx = 0;
    uint32_t now_idx = trace_idx;
    uint32_t dly=0, dlyu=0, blk=0;

    for (uint32_t i = last_idx; i < now_idx; ++i) {
        BlockEvt *e = &trace_buf[i % TRACE_BUF_SZ];
        if      (!strcmp(e->reason, "DELAY"))        dly++;
        else if (!strcmp(e->reason, "DELAY_UNTIL"))  dlyu++;
        else if (!strcmp(e->reason, "BLOCK_Q_RECV") ||
                 !strcmp(e->reason, "BLOCK_Q_PEEK")) blk++;
    }
    ESP_LOGI("TRACE", "last 1s: DELAY=%u DELAY_UNTIL=%u BLOCK_ON_LOCK=%u", dly, dlyu, blk);
    last_idx = now_idx;
}

/* ----- GPIO helpers ----- */
static void led_init(void) {
    gpio_config_t io = (gpio_config_t){0};
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = 1ULL << LED_PIN;
    gpio_config(&io);
    gpio_set_level(LED_PIN, 0); /* start OFF (flip if active-low board) */
}
static inline void led_on(void)  { gpio_set_level(LED_PIN, 1); }
static inline void led_off(void) { gpio_set_level(LED_PIN, 0); }

/* ===== Tasks per assignment =====
   T1: LED ON, actively wait 0.5 s, then yield; also block 1 tick to ensure progress.
   T2: LED OFF, delay 1 s.
   T3: status every 1 s + trace summary print.
*/

/* T1 */
static void task_led_on(void *arg) {
    (void)arg;
    const TickType_t half_sec = pdMS_TO_TICKS(500);

    for (;;) {
        TickType_t t0 = xTaskGetTickCount();
        xSemaphoreTake(g_ledLock, portMAX_DELAY);
        TickType_t t1 = xTaskGetTickCount();

        ESP_LOGI(TAG, "T1: took LED %s (wait=%lu ms)", g_lockKind,
                 (unsigned long)((t1 - t0) * portTICK_PERIOD_MS));

        led_on();
        ESP_LOGI(TAG, "T1: LED ON (busy-wait 500 ms)");
        xSemaphoreGive(g_ledLock);

        /* active wait (do NOT hold the lock) */
        TickType_t start = xTaskGetTickCount();
        while ((xTaskGetTickCount() - start) < half_sec) { /* spin */ }

        /* match spec + guarantee progress for lower priorities */
        taskYIELD();
        vTaskDelay(1);
    }
}

/* T2 */
static void task_led_off(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);

    for (;;) {
        TickType_t t0 = xTaskGetTickCount();
        xSemaphoreTake(g_ledLock, portMAX_DELAY);
        TickType_t t1 = xTaskGetTickCount();

        ESP_LOGI(TAG, "T2: took LED %s (wait=%lu ms)", g_lockKind,
                 (unsigned long)((t1 - t0) * portTICK_PERIOD_MS));

        led_off();
        ESP_LOGI(TAG, "T2: LED OFF (delay 1000 ms)");
        xSemaphoreGive(g_ledLock);

        vTaskDelay(one_sec);
    }
}

/* T3 */
static void task_status(void *arg) {
    (void)arg;
    const TickType_t one_sec = pdMS_TO_TICKS(1000);
    static int first = 1;

    for (;;) {
        if (first) { ESP_LOGI(TAG, "Status: lock=%s, SAME_PRIORITY=%d", g_lockKind, SAME_PRIORITY); first = 0; }
        dump_trace_summary();
        ESP_LOGI(TAG, "T3: tick=%lu", (unsigned long)xTaskGetTickCount());
        vTaskDelay(one_sec);
    }
}

/* ===== App entry ===== */
void app_main(void) {
    ESP_LOGI(TAG, "app_main: init");
    led_init();

#if USE_MUTEX
    g_ledLock = xSemaphoreCreateMutex();              /* (c) PI enabled */
#else
    g_ledLock = xSemaphoreCreateBinary();             /* (b) NO PI */
    xSemaphoreGive(g_ledLock);                        /* start unlocked */
#endif
    configASSERT(g_ledLock != NULL);

    xTaskCreate(task_led_on,  "tLED_ON",  1024, NULL, PRIO_TASK1_LED_ON,  NULL);
    xTaskCreate(task_led_off, "tLED_OFF", 1024, NULL, PRIO_TASK2_LED_OFF, NULL);
    xTaskCreate(task_status,  "tSTATUS",  1024, NULL, PRIO_TASK3_STATUS,  NULL);
}