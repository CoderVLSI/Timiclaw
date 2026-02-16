#ifndef AGENT_LOOP_H
#define AGENT_LOOP_H

#include <Arduino.h>

void agent_loop_init();
void agent_loop_tick();

// Get/set the last LLM response (for emailing code)
String agent_loop_get_last_response();
void agent_loop_set_last_response(const String &response);

#endif
