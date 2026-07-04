#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ====================== Core kernel ====================== */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      133000000 // Tần số mặc định RP2040
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                (configSTACK_DEPTH_TYPE)256
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* ====================== SMP (RP2040 lõi kép) ======================
 * FreeRTOS-Kernel bản mới nhất bạn clone dùng port SMP cho RP2040
 * (thấy qua cờ biên dịch FREE_RTOS_KERNEL_SMP=1). Các define dưới đây
 * BẮT BUỘC phải có, nếu không FreeRTOS.h sẽ báo lỗi #error.
 */
#define configNUMBER_OF_CORES                   1   // để 1 nếu chỉ chạy task trên 1 core (đơn giản, an toàn khi mới bắt đầu)
#define configTICK_CORE                         0
#define configRUN_MULTIPLE_PRIORITIES           1
#define configUSE_CORE_AFFINITY                 0
#define configUSE_PASSIVE_IDLE_HOOK             0
#define configSUPPORT_PICO_SYNC_INTEROP         1   // cho phép dùng chung mutex/semaphore giữa SDK và FreeRTOS
#define configSUPPORT_PICO_TIME_INTEROP         1   // cho phép dùng chung absolute_time_t giữa SDK và FreeRTOS

/* ====================== Bộ nhớ ====================== */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   (20 * 1024) // 20KB Heap cho FreeRTOS

/* ====================== Hook functions ======================
 * BẮT BUỘC phải định nghĩa, nếu không FreeRTOS.h báo lỗi #error.
 * Đặt = 0 nghĩa là không dùng hook, không cần viết hàm callback tương ứng.
 */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2   // bật kiểm tra tràn stack (hữu ích khi debug)
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ====================== Thống kê runtime (tắt để tiết kiệm bộ nhớ) ====================== */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ====================== Co-routine (không dùng) ====================== */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2

/* ====================== Software timer ====================== */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024

/* ====================== Optional API - bật hàm nào dùng hàm đó ======================
 * QUAN TRỌNG: phải có tiền tố "INCLUDE_", nếu không sẽ bị trùng tên với
 * chính macro/hàm thật của FreeRTOS (gây lỗi "redefined" như bạn gặp).
 */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskDelayUntil                 1   // cho phép dùng vTaskDelayUntil / xTaskDelayUntil
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xTaskAbortDelay                  1
#define INCLUDE_xTaskGetHandle                   1
#define INCLUDE_xTaskResumeFromISR               1

#endif /* FREERTOS_CONFIG_H */