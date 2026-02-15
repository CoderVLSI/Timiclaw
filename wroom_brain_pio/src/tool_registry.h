#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include <Arduino.h>

void tool_registry_init();
bool tool_registry_execute(const String &input, String &out);

#endif
