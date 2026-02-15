#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "transport_telegram.h"

void scheduler_init(void);
void scheduler_tick(incoming_cb_t dispatch_cb);

#endif
