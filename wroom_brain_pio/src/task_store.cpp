#include "task_store.h"

#include <Arduino.h>
#include <Preferences.h>

#include "brain_config.h"

namespace {

Preferences g_prefs;
bool g_ready = false;
const char *kNamespace = "braintasks";
const char *kTasksKey = "tasks";
const char *kNextIdKey = "nextid";

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

String sanitize_text(String text) {
  text.replace('\r', ' ');
  text.replace('\n', ' ');
  text.replace('|', '/');
  text.trim();
  if (text.length() > 180) {
    text = text.substring(0, 180);
  }
  return text;
}

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

bool parse_line(const String &line, int &id_out, int &done_out, String &text_out) {
  const int p1 = line.indexOf('|');
  if (p1 <= 0) {
    return false;
  }
  const int p2 = line.indexOf('|', p1 + 1);
  if (p2 <= p1) {
    return false;
  }
  id_out = line.substring(0, p1).toInt();
  done_out = line.substring(p1 + 1, p2).toInt();
  text_out = line.substring(p2 + 1);
  text_out.trim();
  return id_out > 0;
}

}  // namespace

void task_store_init() {
  String err;
  if (ensure_ready(err)) {
    Serial.println("[tasks] NVS tasks ready");
  } else {
    Serial.println("[tasks] init failed");
  }
}

bool task_add(const String &text, int &task_id_out, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String clean = sanitize_text(text);
  if (clean.length() == 0) {
    error_out = "empty task";
    return false;
  }

  unsigned int next_id = g_prefs.getUInt(kNextIdKey, 1);
  String tasks = g_prefs.getString(kTasksKey, "");
  tasks += String(next_id) + "|0|" + clean + "\n";
  tasks = trim_oldest_lines(tasks, TASKS_MAX_CHARS);

  size_t written = g_prefs.putString(kTasksKey, tasks);
  if (written == 0 && tasks.length() > 0) {
    error_out = "failed to store task";
    return false;
  }
  g_prefs.putUInt(kNextIdKey, next_id + 1);
  task_id_out = (int)next_id;
  return true;
}

bool task_list(String &list_out, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String tasks = g_prefs.getString(kTasksKey, "");
  tasks.trim();
  if (tasks.length() == 0) {
    list_out = "No tasks";
    return true;
  }

  String out = "Tasks:\n";
  bool any = false;
  int start = 0;
  while (start < (int)tasks.length()) {
    int end = tasks.indexOf('\n', start);
    if (end < 0) {
      end = tasks.length();
    }
    String line = tasks.substring(start, end);
    line.trim();
    if (line.length() > 0) {
      int id = 0;
      int done = 0;
      String text;
      if (parse_line(line, id, done, text)) {
        out += String(done == 1 ? "[x] " : "[ ] ");
        out += "#" + String(id) + " " + text + "\n";
        any = true;
      }
    }
    start = end + 1;
  }

  if (!any) {
    list_out = "No tasks";
    return true;
  }

  if (out.length() > 1400) {
    out = out.substring(out.length() - 1400);
  }
  list_out = out;
  return true;
}

bool task_done(int task_id, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }
  if (task_id <= 0) {
    error_out = "invalid task id";
    return false;
  }

  String tasks = g_prefs.getString(kTasksKey, "");
  String rewritten;
  rewritten.reserve(tasks.length() + 32);
  bool found = false;

  int start = 0;
  while (start < (int)tasks.length()) {
    int end = tasks.indexOf('\n', start);
    if (end < 0) {
      end = tasks.length();
    }
    String line = tasks.substring(start, end);
    line.trim();
    if (line.length() > 0) {
      int id = 0;
      int done = 0;
      String text;
      if (parse_line(line, id, done, text)) {
        if (id == task_id) {
          done = 1;
          found = true;
        }
        rewritten += String(id) + "|" + String(done) + "|" + sanitize_text(text) + "\n";
      }
    }
    start = end + 1;
  }

  if (!found) {
    error_out = "task id not found";
    return false;
  }

  size_t written = g_prefs.putString(kTasksKey, rewritten);
  if (written == 0 && rewritten.length() > 0) {
    error_out = "failed to update task";
    return false;
  }
  return true;
}

bool task_clear(String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }
  g_prefs.remove(kTasksKey);
  return true;
}
