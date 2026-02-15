#include "memory_store.h"

#include <Arduino.h>
#include <Preferences.h>

#include "brain_config.h"

namespace {

Preferences g_prefs;
bool g_ready = false;
const char *kNamespace = "brainmem";
const char *kNotesKey = "notes";

String trim_oldest_lines(String text, size_t max_chars) {
  if (text.length() <= max_chars) {
    return text;
  }

  while (text.length() > max_chars) {
    int nl = text.indexOf('\n');
    if (nl < 0) {
      return text.substring(text.length() - max_chars);
    }
    text = text.substring(nl + 1);
  }

  return text;
}

bool ensure_ready(String &error_out) {
  if (g_ready) {
    return true;
  }
  if (!g_prefs.begin(kNamespace, false)) {
    error_out = "NVS begin failed";
    return false;
  }
  g_ready = true;
  return true;
}

}  // namespace

void memory_init() {
  String err;
  if (ensure_ready(err)) {
    Serial.println("[memory] NVS memory ready");
  } else {
    Serial.println("[memory] init failed");
  }
}

bool memory_append_note(const String &note, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String cleaned = note;
  cleaned.trim();
  if (cleaned.length() == 0) {
    error_out = "empty memory note";
    return false;
  }

  String existing = g_prefs.getString(kNotesKey, "");
  String merged = existing;
  if (merged.length() > 0 && !merged.endsWith("\n")) {
    merged += "\n";
  }
  merged += "- " + cleaned + "\n";

  merged = trim_oldest_lines(merged, MEMORY_MAX_CHARS);

  size_t written = g_prefs.putString(kNotesKey, merged);
  if (written == 0 && merged.length() > 0) {
    error_out = "failed to write memory";
    return false;
  }
  return true;
}

bool memory_get_notes(String &notes_out, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  notes_out = g_prefs.getString(kNotesKey, "");
  return true;
}

bool memory_clear_notes(String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }
  g_prefs.remove(kNotesKey);
  return true;
}
