#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>

void tool_registry_init(void);
bool tool_registry_execute(const char *input, char *out, size_t out_len);

#endif
