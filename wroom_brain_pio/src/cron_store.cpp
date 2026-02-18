#include "cron_store.h"

#include <FS.h>
#include "SPIFFS.h"

#define CRON_FILENAME "/cron.md"

// Cache of loaded jobs
static CronJob s_cached_jobs[CRON_MAX_JOBS];
static int s_cached_count = 0;
static bool s_initialized = false;

// Load all cron jobs from cron.md
static void cron_store_load() {
  s_cached_count = 0;

  if (!SPIFFS.exists(CRON_FILENAME)) {
    // Create default cron.md with header
    File f = SPIFFS.open(CRON_FILENAME, "w");
    if (f) {
      f.println("# Cron Jobs");
      f.println("# Format: minute hour day month weekday | command");
      f.println("# Example: 0 9 * * * | Good morning message");
      f.println("# Wildcards: * means any value");
      f.println("# minute: 0-59, hour: 0-23, day: 1-31, month: 1-12, weekday: 0-6 (0=Sunday)");
      f.println("");
      f.close();
    }
    return;
  }

  File f = SPIFFS.open(CRON_FILENAME, "r");
  if (!f) {
    Serial.println("[cron_store] Failed to open cron.md");
    return;
  }

  Serial.println("[cron_store] Loading cron jobs from cron.md");

  while (f.available() && s_cached_count < CRON_MAX_JOBS) {
    String line = f.readStringUntil('\n');
    line.trim();

    // Remove carriage return if present
    if (line.length() > 0 && line[line.length() - 1] == '\r') {
      line.remove(line.length() - 1);
    }

    if (line.length() == 0) {
      continue;
    }

    // Skip comments
    if (line[0] == '#') {
      continue;
    }

    CronJob job;
    String error;
    if (cron_parse_line(line, job, error)) {
      s_cached_jobs[s_cached_count++] = job;
      Serial.printf("[cron_store] Loaded: %s\n", cron_job_to_string(job).c_str());
    } else {
      Serial.printf("[cron_store] Skipping invalid line: %s (error: %s)\n", line.c_str(), error.c_str());
    }
  }

  f.close();
  Serial.printf("[cron_store] Loaded %d cron job(s)\n", s_cached_count);
}

void cron_store_init() {
  if (s_initialized) {
    return;
  }

  if (!SPIFFS.begin(true)) {
    Serial.println("[cron_store] SPIFFS mount failed");
    return;
  }

  cron_store_load();
  s_initialized = true;
}

bool cron_store_add(const String &cron_line, String &error_out) {
  // Validate the line first
  CronJob job;
  if (!cron_parse_line(cron_line, job, error_out)) {
    return false;
  }

  if (s_cached_count >= CRON_MAX_JOBS) {
    error_out = "Maximum cron jobs reached (" + String(CRON_MAX_JOBS) + ")";
    return false;
  }

  // Add to cache
  s_cached_jobs[s_cached_count++] = job;

  // Append to file
  File f = SPIFFS.open(CRON_FILENAME, "a");
  if (!f) {
    error_out = "Failed to open cron.md for writing";
    return false;
  }

  f.println(cron_line);
  f.close();

  Serial.printf("[cron_store] Added cron job: %s\n", cron_job_to_string(job).c_str());
  return true;
}

int cron_store_get_all(CronJob *jobs, int max_jobs) {
  int count = (s_cached_count < max_jobs) ? s_cached_count : max_jobs;
  for (int i = 0; i < count; i++) {
    jobs[i] = s_cached_jobs[i];
  }
  return count;
}

bool cron_store_clear(String &error_out) {
  s_cached_count = 0;

  // Rewrite file with header only
  File f = SPIFFS.open(CRON_FILENAME, "w");
  if (!f) {
    error_out = "Failed to open cron.md for writing";
    return false;
  }

  f.println("# Cron Jobs");
  f.println("# Format: minute hour day month weekday | command");
  f.println("# Example: 0 9 * * * | Good morning message");
  f.println("");
  f.close();

  Serial.println("[cron_store] Cleared all cron jobs");
  return true;
}

bool cron_store_get_content(String &content_out, String &error_out) {
  if (!SPIFFS.exists(CRON_FILENAME)) {
    error_out = "cron.md does not exist";
    return false;
  }

  File f = SPIFFS.open(CRON_FILENAME, "r");
  if (!f) {
    error_out = "Failed to open cron.md";
    return false;
  }

  content_out = f.readString();
  f.close();
  return true;
}

int cron_store_count() {
  return s_cached_count;
}
