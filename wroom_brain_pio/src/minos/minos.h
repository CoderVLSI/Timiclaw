#ifndef MINOS_H
#define MINOS_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>

/* Task states */
#define TASK_READY    0
#define TASK_RUNNING  1
#define TASK_SLEEPING 2
#define TASK_BLOCKED  3

/* Maximum number of tasks */
#define MAX_TASKS 16

/* Task Control Block */
typedef struct task {
    uint32_t id;
    char name[32];
    uint8_t state;
    uint8_t priority;
    void (*entry)(void);
    uint32_t stack_ptr;
    uint32_t stack_size;
    uint32_t sleep_ticks;
} task_t;

/* Kernel functions */
void kernel_init(void);
void kernel_start(void);
void kernel_yield(void);
void kernel_sleep(uint32_t ticks);
void kernel_stop(void);

/* Task functions */
int task_create(const char *name, void (*entry)(void), uint8_t priority);
void task_exit(void);
uint32_t task_get_id(void);

/* Time functions */
uint32_t ticks_get(void);
void delay_ms(uint32_t ms);

/* Shell functions */
void shell_init(void);
void shell_run_once(const String &input, String &output);

/* Hardware abstraction (Virtual UART for Agent) */
void hw_init(void);
void hw_print(const String &s);
void hw_println(const String &s);

/* Exported kernel variables */
extern task_t tasks[MAX_TASKS];
extern uint32_t task_count;
extern bool kernel_running;

#endif
