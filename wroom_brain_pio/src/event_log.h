#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <Arduino.h>

void event_log_init();
void event_log_append(const String &line);
void event_log_clear();
void event_log_dump(String &out, size_t max_chars);

#endif
