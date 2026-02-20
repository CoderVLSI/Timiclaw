#include "minos.h"

task_t tasks[MAX_TASKS];
uint32_t task_count = 0;
bool kernel_running = false;
static uint32_t current_task = 0;
static uint32_t system_ticks = 0;

void kernel_init(void) {
    memset(tasks, 0, sizeof(tasks));
    task_count = 0;
    current_task = 0;
    system_ticks = 0;
    kernel_running = false;
    hw_init();
    Serial.println("[MinOS] Kernel initialized.");
}

int task_create(const char *name, void (*entry)(void), uint8_t priority) {
    if (task_count >= MAX_TASKS) return -1;
    task_t *task = &tasks[task_count];
    task->id = task_count;
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->entry = entry;
    task->priority = priority;
    task->state = TASK_READY;
    task->sleep_ticks = 0;
    return task_count++;
}

void kernel_sleep(uint32_t ticks) {
    if (current_task < task_count) {
        tasks[current_task].state = TASK_SLEEPING;
        tasks[current_task].sleep_ticks = ticks;
    }
}

void kernel_yield(void) {
    // In this simple cooperative loop, yielding is just returning from entry()
}

uint32_t ticks_get(void) {
    return system_ticks;
}

void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void kernel_stop(void) {
    kernel_running = false;
}

static void tick_handler(void) {
    system_ticks++;
    for (uint32_t i = 0; i < task_count; i++) {
        if (tasks[i].state == TASK_SLEEPING) {
            if (tasks[i].sleep_ticks > 0) {
                tasks[i].sleep_ticks--;
                if (tasks[i].sleep_ticks == 0) {
                    tasks[i].state = TASK_READY;
                }
            }
        }
    }
}

void kernel_start(void) {
    kernel_running = true;
    Serial.println("[MinOS] Kernel started.");

    while (kernel_running) {
        for (uint32_t i = 0; i < task_count; i++) {
            if (tasks[i].state == TASK_READY) {
                current_task = i;
                tasks[i].state = TASK_RUNNING;
                if (tasks[i].entry) {
                    tasks[i].entry();
                }
                tasks[i].state = TASK_READY;
            }
        }
        tick_handler();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms tick
    }
}
