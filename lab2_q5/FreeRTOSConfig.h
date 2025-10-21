/*
 * Lab-friendly FreeRTOSConfig.h for ESP8266/ESP-IDF v3.x
 * Enables: preemption, NO timeslicing (so equal priorities are run-to-completion),
 * trace facility with lightweight block tracing hooks.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "sdkconfig.h"

#ifndef __ASSEMBLER__
#include <stdlib.h>
#endif

/* ---- Core scheduling ---- */
#define portNUM_PROCESSORS                  1
#define configUSE_PREEMPTION                1
/* SAME-PRIORITY experiment: run-to-completion (switch only on block/yield) */
#define configUSE_TIME_SLICING              0

/* Hooks OFF unless implemented */
#define configUSE_IDLE_HOOK                 0
#define configUSE_TICK_HOOK                 0

/* Clock / tick */
#define configCPU_CLOCK_HZ                  ( 80000000UL )
#define configTICK_RATE_HZ                  ( (TickType_t) CONFIG_FREERTOS_HZ )

/* Priorities / stacks */
#define configMAX_PRIORITIES                15
#define configMINIMAL_STACK_SIZE            ( ( unsigned short ) 768 )
#define configMAX_TASK_NAME_LEN             16
#define configUSE_16_BIT_TICKS              0
#define configIDLE_SHOULD_YIELD             1

/* Safety / debug */
#define configCHECK_FOR_STACK_OVERFLOW      2
#define INCLUDE_xTaskGetIdleTaskHandle      1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xSemaphoreGetMutexHolder    1

/* ----- Sync primitives ----- */
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         1
#define configUSE_COUNTING_SEMAPHORES       1
/* Priority Inheritance is enabled by using MUTEXes. */
#define configUSE_PRIORITY_INHERITANCE      1

/* Timers (fine to keep) */
#define configUSE_TIMERS                    1
#if configUSE_TIMERS
  #define configTIMER_TASK_PRIORITY         ( tskIDLE_PRIORITY + 2 )
  #define configTIMER_QUEUE_LENGTH          10
  #define configTIMER_TASK_STACK_DEPTH      ( ( unsigned short ) CONFIG_FREERTOS_TIMER_STACKSIZE )
  #define INCLUDE_xTimerPendFunctionCall    1
#endif

/* Optional / stats */
#define configUSE_TRACE_FACILITY            1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configGENERATE_RUN_TIME_STATS       0

/* Interrupt priority placeholders (Cortex-style constants; ESP8266 doesn’t use NVIC) */
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15

#ifndef configIDLE_TASK_STACK_SIZE
  #define configIDLE_TASK_STACK_SIZE        CONFIG_FREERTOS_IDLE_TASK_STACKSIZE
#endif

/* -------- Lightweight block tracing hooks -------- */
#ifdef __cplusplus
extern "C" {
#endif
void app_trace_record(const char *reason);   /* Implemented in app_main.c */
#ifdef __cplusplus
}
#endif

/* Fires whenever a task blocks due to a delay */
#define traceTASK_DELAY()                        app_trace_record("DELAY")
#define traceTASK_DELAY_UNTIL(xTimeToWake)       app_trace_record("DELAY_UNTIL")
/* Fires when a task is about to block waiting on a queue/sem/mutex */
#define traceBLOCKING_ON_QUEUE_RECEIVE(pxQueue)  app_trace_record("BLOCK_Q_RECV")
#define traceBLOCKING_ON_QUEUE_PEEK(pxQueue)     app_trace_record("BLOCK_Q_PEEK")

#endif /* FREERTOS_CONFIG_H */