/*
 * Minimal, lab-friendly FreeRTOSConfig.h for ESP8266/ESP-IDF v3.x
 * Works for:
 *   - SAME PRIORITY (run-to-completion)
 *   - (a) T1>T2>T3 run-to-completion
 *   - (b) No PI  (use BINARY SEMAPHORE in code)
 *   - (c) PI enabled (use MUTEX in code)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "sdkconfig.h"

#ifndef __ASSEMBLER__
#include <stdlib.h>
#endif

/* ----- Core scheduling behavior ----- */
#define portNUM_PROCESSORS              1
#define configUSE_PREEMPTION            1
/* Run-to-completion for equal priority tasks: */
#define configUSE_TIME_SLICING          0

/* Keep hooks OFF unless you implement them */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0

/* Tick and clocks */
#define configCPU_CLOCK_HZ              ( 80000000UL )
#define configTICK_RATE_HZ              ( (TickType_t) CONFIG_FREERTOS_HZ )

/* Priorities and stacks */
#define configMAX_PRIORITIES            15
#define configMINIMAL_STACK_SIZE        ( ( unsigned short ) 768 )
#define configMAX_TASK_NAME_LEN         16
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1

/* Safety / debug */
#define configCHECK_FOR_STACK_OVERFLOW  2
#define INCLUDE_xTaskGetIdleTaskHandle  1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskSuspend            1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1
#define INCLUDE_xSemaphoreGetMutexHolder 1

/* ----- Synchronization primitives ----- */
/* Mutexes (needed for PI) */
#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     1
/* Make PI explicit (many ports enable it with mutexes; we force it on) */
#define configUSE_PRIORITY_INHERITANCE  1
/* Counting semaphores on for generality */
#define configUSE_COUNTING_SEMAPHORES   1

/* Software timers (fine to keep) */
#define configUSE_TIMERS                1
#if configUSE_TIMERS
  #define configTIMER_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
  #define configTIMER_QUEUE_LENGTH      10
  #define configTIMER_TASK_STACK_DEPTH  ( ( unsigned short ) CONFIG_FREERTOS_TIMER_STACKSIZE )
  #define INCLUDE_xTimerPendFunctionCall 1
#endif

/* Optional features you can keep enabled/disabled as you like */
#define configUSE_CO_ROUTINES           0
#define configMAX_CO_ROUTINE_PRIORITIES 2
#define configUSE_TRACE_FACILITY        0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configGENERATE_RUN_TIME_STATS   0

/* Interrupt priority placeholders (not used on ESP8266 like Cortex-M) */
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15

/* Idle task stack per ESP-IDF menuconfig */
#ifndef configIDLE_TASK_STACK_SIZE
  #define configIDLE_TASK_STACK_SIZE    CONFIG_FREERTOS_IDLE_TASK_STACKSIZE
#endif

#endif /* FREERTOS_CONFIG_H */