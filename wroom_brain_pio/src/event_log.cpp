#include "event_log.h"

#include <Arduino.h>

namespace {

static const int kMaxEntries = 40;
String g_entries[kMaxEntries];
int g_head = 0;
int g_count = 0;

String sanitize_line(const String &in) {
  String out = in;
  out.replace('\r', ' ');
  out.replace('\n', ' ');
  out.trim();
  if (out.length() > 180) {
    out = out.substring(0, 180) + "...";
  }
  return out;
}

}  // namespace

void event_log_init() {
  g_head = 0;
  g_count = 0;
}

void event_log_append(const String &line) {
  String entry = String("[") + String(millis() / 1000UL) + "s] " + sanitize_line(line);

  if (g_count < kMaxEntries) {
    int idx = (g_head + g_count) % kMaxEntries;
    g_entries[idx] = entry;
    g_count++;
    return;
  }

  g_entries[g_head] = entry;
  g_head = (g_head + 1) % kMaxEntries;
}

void event_log_clear() {
  g_head = 0;
  g_count = 0;
}

void event_log_dump(String &out, size_t max_chars) {
  if (g_count == 0) {
    out = "Logs are empty";
    return;
  }

  out = "Logs:\n";
  for (int i = 0; i < g_count; i++) {
    int idx = (g_head + i) % kMaxEntries;
    out += g_entries[idx];
    out += '\n';
  }

  if (out.length() > max_chars) {
    out = out.substring(out.length() - max_chars);
  }
}
