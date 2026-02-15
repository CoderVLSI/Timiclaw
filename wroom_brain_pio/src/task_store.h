#ifndef TASK_STORE_H
#define TASK_STORE_H

#include <Arduino.h>

void task_store_init();
bool task_add(const String &text, int &task_id_out, String &error_out);
bool task_list(String &list_out, String &error_out);
bool task_done(int task_id, String &error_out);
bool task_clear(String &error_out);

#endif
