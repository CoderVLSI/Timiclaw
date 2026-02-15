#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include "transport_telegram.h"

void scheduler_init();
void scheduler_tick(incoming_cb_t dispatch_cb);
void scheduler_time_debug(String &out);

#endif
