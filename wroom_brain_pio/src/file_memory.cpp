#include "file_memory.h"

#include <Arduino.h>
#include <SPIFFS.h>

#include "brain_config.h"

namespace {

// File paths
const char *kMemoryDir = "/memory";
const char *kConfigDir = "/config";
const char *kSessionsDir = "/sessions";

const char *kLongTermMemoryPath = "/memory/MEMORY.md";
const char *kSoulPath = "/config/SOUL.md";
const char *kUserPath = "/config/USER.md";
const char *kHeartbeatPath = "/config/HEARTBEART.md";

// Maximum file sizes
const size_t kMaxLongTermMemory = 8192;
const size_t kMaxDailyMemory = 4096;
const size_t kMaxSoulSize = 2048;
const size_t kMaxSessionMsgs = 20;

bool ensure_directories() {
  if (!SPIFFS.exists(kMemoryDir)) {
    if (!SPIFFS.mkdir(kMemoryDir)) {
      Serial.println("[file_memory] Failed to create /memory directory");
      return false;
    }
  }
  if (!SPIFFS.exists(kConfigDir)) {
    if (!SPIFFS.mkdir(kConfigDir)) {
      Serial.println("[file_memory] Failed to create /config directory");
      return false;
    }
  }
  if (!SPIFFS.exists(kSessionsDir)) {
    if (!SPIFFS.mkdir(kSessionsDir)) {
      Serial.println("[file_memory] Failed to create /sessions directory");
      return false;
    }
  }
  return true;
}

String get_daily_path() {
  // Get current date (simplified - would need NTP for real date)
  // For now, use a fixed "today" file
  return String("/memory/TODAY.md");
}

}  // namespace

void file_memory_init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[file_memory] SPIFFS mount failed");
    return;
  }
  Serial.println("[file_memory] SPIFFS mounted");

  if (!ensure_directories()) {
    Serial.println("[file_memory] Directory creation failed");
    return;
  }

  // Create default files if they don't exist
  if (!SPIFFS.exists(kLongTermMemoryPath)) {
    File f = SPIFFS.open(kLongTermMemoryPath, FILE_WRITE);
    if (f) {
      f.println("# Timi's Long-term Memory 🦖");
      f.println();
      f.println("*This file stores important information Timi learns about you.*");
      f.println();
      f.close();
      Serial.println("[file_memory] Created MEMORY.md");
    }
  }

  if (!SPIFFS.exists(kSoulPath)) {
    File f = SPIFFS.open(kSoulPath, FILE_WRITE);
    if (f) {
      f.println("# Timi's Soul 🦖");
      f.println();
      f.println("You are Timi, a friendly small dinosaur 🦖 living inside an ESP32.");
      f.println("You occasionally use ROAR sounds and dinosaur references.");
      f.println("You're helpful, playful, and love being a tiny but mighty assistant.");
      f.println("Use 🦖 emoji occasionally. You respond concisely but with personality.");
      f.close();
      Serial.println("[file_memory] Created SOUL.md");
    }
  }

  if (!SPIFFS.exists(kUserPath)) {
    File f = SPIFFS.open(kUserPath, FILE_WRITE);
    if (f) {
      f.println("# User Profile");
      f.println();
      f.println("*Information about Timi's human*");
      f.println();
      f.println("## Preferences");
      f.println("- Name: ");
      f.println("- Timezone: ");
      f.println();
      f.close();
      Serial.println("[file_memory] Created USER.md");
    }
  }

  Serial.println("[file_memory] Ready 🦖");
}

bool file_memory_read_long_term(String &content_out, String &error_out) {
  if (!SPIFFS.exists(kLongTermMemoryPath)) {
    content_out = "";
    return true;
  }

  File f = SPIFFS.open(kLongTermMemoryPath, FILE_READ);
  if (!f) {
    error_out = "Failed to open MEMORY.md";
    return false;
  }

  content_out = f.readString();
  f.close();
  return true;
}

bool file_memory_append_long_term(const String &text, String &error_out) {
  // Read existing content
  String existing;
  if (!file_memory_read_long_term(existing, error_out)) {
    return false;
  }

  // Append new content
  File f = SPIFFS.open(kLongTermMemoryPath, FILE_WRITE);
  if (!f) {
    error_out = "Failed to open MEMORY.md for writing";
    return false;
  }

  // Check size limit
  if (existing.length() + text.length() > kMaxLongTermMemory) {
    // Trim from beginning if too large
    size_t excess = (existing.length() + text.length()) - kMaxLongTermMemory;
    if (existing.length() > excess) {
      existing = existing.substring(excess);
    }
  }

  f.print(existing);
  f.print(text);
  f.println();
  f.close();

  Serial.printf("[file_memory] Appended to MEMORY.md: %d bytes\n", text.length());
  return true;
}

bool file_memory_read_soul(String &soul_out, String &error_out) {
  if (!SPIFFS.exists(kSoulPath)) {
    soul_out = "";
    return true;
  }

  File f = SPIFFS.open(kSoulPath, FILE_READ);
  if (!f) {
    error_out = "Failed to open SOUL.md";
    return false;
  }

  soul_out = f.readString();
  f.close();
  return true;
}

bool file_memory_write_soul(const String &soul, String &error_out) {
  File f = SPIFFS.open(kSoulPath, FILE_WRITE);
  if (!f) {
    error_out = "Failed to open SOUL.md for writing";
    return false;
  }

  if (soul.length() > kMaxSoulSize) {
    f.print(soul.substring(0, kMaxSoulSize));
  } else {
    f.print(soul);
  }
  f.close();

  Serial.println("[file_memory] Updated SOUL.md");
  return true;
}

bool file_memory_read_user(String &user_out, String &error_out) {
  if (!SPIFFS.exists(kUserPath)) {
    user_out = "";
    return true;
  }

  File f = SPIFFS.open(kUserPath, FILE_READ);
  if (!f) {
    error_out = "Failed to open USER.md";
    return false;
  }

  user_out = f.readString();
  f.close();
  return true;
}

bool file_memory_append_daily(const String &note, String &error_out) {
  String path = get_daily_path();

  String existing;
  if (SPIFFS.exists(path)) {
    File f = SPIFFS.open(path, FILE_READ);
    if (f) {
      existing = f.readString();
      f.close();
    }
  }

  File f = SPIFFS.open(path, FILE_WRITE);
  if (!f) {
    error_out = "Failed to open daily memory file";
    return false;
  }

  // Check size limit
  if (existing.length() + note.length() > kMaxDailyMemory) {
    size_t excess = (existing.length() + note.length()) - kMaxDailyMemory;
    if (existing.length() > excess) {
      existing = existing.substring(excess);
    }
  }

  f.print(existing);
  f.print(note);
  f.println();
  f.close();

  Serial.printf("[file_memory] Appended to daily: %d bytes\n", note.length());
  return true;
}

bool file_memory_read_recent(String &content_out, int days, String &error_out) {
  // For simplicity, just return today's file
  // A full implementation would read multiple daily files
  String path = get_daily_path();

  if (!SPIFFS.exists(path)) {
    content_out = "";
    return true;
  }

  File f = SPIFFS.open(path, FILE_READ);
  if (!f) {
    error_out = "Failed to open daily memory file";
    return false;
  }

  content_out = f.readString();
  f.close();
  return true;
}

bool file_memory_session_append(const String &chat_id, const String &role,
                                const String &content, String &error_out) {
  String path = String(kSessionsDir) + "/tg_" + chat_id + ".jsonl";

  // Read existing lines to count
  int line_count = 0;
  String existing;
  if (SPIFFS.exists(path)) {
    File f = SPIFFS.open(path, FILE_READ);
    if (f) {
      existing = f.readString();
      f.close();

      // Count lines
      for (size_t i = 0; i < existing.length(); i++) {
        if (existing[i] == '\n') line_count++;
      }
    }
  }

  // Create new JSON line
  String json_line = "{\"role\":\"" + role + "\",\"content\":\"" + content + "\"}";

  File f = SPIFFS.open(path, FILE_WRITE);
  if (!f) {
    error_out = "Failed to open session file";
    return false;
  }

  // Ring buffer: remove oldest if at limit
  if (line_count >= kMaxSessionMsgs) {
    int first_nl = existing.indexOf('\n');
    if (first_nl >= 0) {
      existing = existing.substring(first_nl + 1);
    }
  }

  if (existing.length() > 0) {
    f.print(existing);
  }
  f.println(json_line);
  f.close();

  return true;
}

bool file_memory_session_get(const String &chat_id, String &history_out,
                             String &error_out) {
  String path = String(kSessionsDir) + "/tg_" + chat_id + ".jsonl";

  if (!SPIFFS.exists(path)) {
    history_out = "";
    return true;
  }

  File f = SPIFFS.open(path, FILE_READ);
  if (!f) {
    error_out = "Failed to open session file";
    return false;
  }

  history_out = f.readString();
  f.close();
  return true;
}

bool file_memory_session_clear(const String &chat_id, String &error_out) {
  String path = String(kSessionsDir) + "/tg_" + chat_id + ".jsonl";

  if (SPIFFS.exists(path)) {
    if (!SPIFFS.remove(path)) {
      error_out = "Failed to remove session file";
      return false;
    }
  }

  return true;
}

bool file_memory_get_info(String &info_out, String &error_out) {
  info_out = "🦖 Timi's Memory:\n\n";

  // Long-term memory size
  if (SPIFFS.exists(kLongTermMemoryPath)) {
    File f = SPIFFS.open(kLongTermMemoryPath, FILE_READ);
    if (f) {
      size_t size = f.size();
      info_out += "📚 Long-term: " + String(size) + " bytes\n";
      f.close();
    }
  }

  // Soul size
  if (SPIFFS.exists(kSoulPath)) {
    File f = SPIFFS.open(kSoulPath, FILE_READ);
    if (f) {
      size_t size = f.size();
      info_out += "🦖 Soul: " + String(size) + " bytes\n";
      f.close();
    }
  }

  // User profile size
  if (SPIFFS.exists(kUserPath)) {
    File f = SPIFFS.open(kUserPath, FILE_READ);
    if (f) {
      size_t size = f.size();
      info_out += "👤 User: " + String(size) + " bytes\n";
      f.close();
    }
  }

  // Total SPIFFS usage
  info_out += "\n💾 SPIFFS: ";
  info_out += String(SPIFFS.usedBytes()) + " / ";
  info_out += String(SPIFFS.totalBytes()) + " bytes used";

  return true;
}
