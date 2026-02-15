#ifndef CHAT_HISTORY_H
#define CHAT_HISTORY_H

#include <Arduino.h>

void chat_history_init();
bool chat_history_append(char role, const String &text, String &error_out);
bool chat_history_get(String &history_out, String &error_out);
bool chat_history_clear(String &error_out);

#endif
