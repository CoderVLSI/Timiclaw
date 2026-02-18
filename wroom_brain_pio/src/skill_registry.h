#ifndef SKILL_REGISTRY_H
#define SKILL_REGISTRY_H

#include <Arduino.h>

// Maximum number of skills indexed at boot
#ifndef SKILL_MAX_COUNT
#define SKILL_MAX_COUNT 15
#endif

// Maximum skill file size (bytes)
#ifndef SKILL_MAX_FILE_SIZE
#define SKILL_MAX_FILE_SIZE 2048
#endif

// Initialize skill registry (scan /skills/ directory)
void skill_init();

// List all indexed skills (name: description)
bool skill_list(String &list_out, String &error_out);

// Match a user query against skill descriptions
// Returns skill name if matched, empty string if none
String skill_match(const String &query);

// Lazy-load full skill content from SPIFFS
bool skill_load(const String &name, String &content_out, String &error_out);

// Show a specific skill's full content
bool skill_show(const String &name, String &content_out, String &error_out);

// Add a new skill (writes .md to /skills/)
bool skill_add(const String &name, const String &description,
               const String &instructions, String &error_out);

// Remove a skill
bool skill_remove(const String &name, String &error_out);

// Get skill descriptions for ReAct agent (compact format)
String skill_get_descriptions_for_react();

#endif
