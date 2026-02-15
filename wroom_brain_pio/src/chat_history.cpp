#include "chat_history.h"

#include <Arduino.h>
#include <Preferences.h>

namespace {

Preferences g_prefs;
bool g_ready = false;

const char *kNamespace = "brainchat";
const char *kKeyLines = "lines";
const int kMaxEntries = 6;      // 6 role-lines ~= 3 user/assistant turns.
const int kMaxLineChars = 160;  // Keep each message compact.
const int kMaxStoreChars = 1000;
const int kMaxOutChars = 420;

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

String compact_spaces(const String &value) {
  String out;
  out.reserve(value.length());
  bool last_space = false;
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    const bool is_space = (c == ' ' || c == '\t' || c == '\r' || c == '\n');
    if (is_space) {
      if (!last_space) {
        out += ' ';
      }
      last_space = true;
    } else {
      out += c;
      last_space = false;
    }
  }
  out.trim();
  return out;
}

int count_lines(const String &text) {
  int lines = 0;
  for (size_t i = 0; i < text.length(); i++) {
    if (text[i] == '\n') {
      lines++;
    }
  }
  return lines;
}

void drop_first_line(String &text) {
  int nl = text.indexOf('\n');
  if (nl < 0) {
    text = "";
    return;
  }
  text = text.substring(nl + 1);
}

String sanitize_text(const String &input) {
  String v = compact_spaces(input);
  if (v.length() > kMaxLineChars) {
    v = v.substring(0, kMaxLineChars);
  }
  return v;
}

}  // namespace

void chat_history_init() {
  String err;
  if (ensure_ready(err)) {
    Serial.println("[chat] NVS history ready");
  } else {
    Serial.println("[chat] init failed");
  }
}

bool chat_history_append(char role, const String &text, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  const char role_norm = (role == 'A' || role == 'a') ? 'A' : 'U';
  String clean = sanitize_text(text);
  if (clean.length() == 0) {
    return true;
  }

  String lines = g_prefs.getString(kKeyLines, "");
  if (lines.length() > 0 && !lines.endsWith("\n")) {
    lines += "\n";
  }
  lines += String(role_norm) + "|" + clean + "\n";

  while (count_lines(lines) > kMaxEntries) {
    drop_first_line(lines);
  }
  while ((int)lines.length() > kMaxStoreChars) {
    drop_first_line(lines);
  }

  size_t written = g_prefs.putString(kKeyLines, lines);
  if (written == 0 && lines.length() > 0) {
    error_out = "failed to write history";
    return false;
  }
  return true;
}

bool chat_history_get(String &history_out, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String lines = g_prefs.getString(kKeyLines, "");
  lines.trim();
  if (lines.length() == 0) {
    history_out = "";
    return true;
  }

  String out;
  out.reserve(lines.length() + 64);

  int start = 0;
  while (start < (int)lines.length()) {
    int end = lines.indexOf('\n', start);
    if (end < 0) {
      end = lines.length();
    }
    String line = lines.substring(start, end);
    int sep = line.indexOf('|');
    if (sep > 0) {
      String role = line.substring(0, sep);
      String text = line.substring(sep + 1);
      text.trim();
      if (text.length() > 0) {
        if (role == "A") {
          out += "Assistant: ";
        } else {
          out += "User: ";
        }
        out += text + "\n";
      }
    }
    start = end + 1;
  }

  out.trim();
  if ((int)out.length() > kMaxOutChars) {
    out = out.substring(out.length() - kMaxOutChars);
    int nl = out.indexOf('\n');
    if (nl >= 0 && nl + 1 < (int)out.length()) {
      out = out.substring(nl + 1);
    }
  }

  history_out = out;
  return true;
}

bool chat_history_clear(String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }
  g_prefs.remove(kKeyLines);
  return true;
}
