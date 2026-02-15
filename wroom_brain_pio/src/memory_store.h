#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#include <Arduino.h>

void memory_init();
bool memory_append_note(const String &note, String &error_out);
bool memory_get_notes(String &notes_out, String &error_out);
bool memory_clear_notes(String &error_out);

#endif
