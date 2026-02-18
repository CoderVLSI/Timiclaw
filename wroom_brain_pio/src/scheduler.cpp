#include "scheduler.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "brain_config.h"
#include "event_log.h"
#include "persona_store.h"
#include "cron_store.h"

static unsigned long s_next_status_ms = 0;
static unsigned long s_next_heartbeat_ms = 0;
static unsigned long s_next_reminder_check_ms = 0;
static bool s_time_configured = false;
static String s_last_tz = "";
static long s_last_tz_offset_seconds = 0;
static int s_last_reminder_yday = -1;
static int s_last_reminder_target_minute = -1;
static int s_last_reminder_reason = -1;

// Cron job tracking
static unsigned long s_next_cron_check_ms = 0;
static int s_last_cron_minute = -1;

static String to_lower_copy(String value) {
  value.toLowerCase();
  return value;
}

static String normalize_tz_for_esp(const String &tz_raw) {
  String tz = tz_raw;
  tz.trim();
  if (tz.length() == 0) {
    return "UTC0";
  }

  String lc = to_lower_copy(tz);
  if (lc == "asia/kolkata" || lc == "asia/calcutta" || lc == "india" || lc == "ist") {
    return "IST-5:30";
  }
  if (lc == "utc" || lc == "etc/utc" || lc == "gmt") {
    return "UTC0";
  }

  // Common US zones (POSIX TZ format with DST rules)
  if (lc == "america/new_york") {
    return "EST5EDT,M3.2.0/2,M11.1.0/2";
  }
  if (lc == "america/chicago") {
    return "CST6CDT,M3.2.0/2,M11.1.0/2";
  }
  if (lc == "america/denver") {
    return "MST7MDT,M3.2.0/2,M11.1.0/2";
  }
  if (lc == "america/los_angeles") {
    return "PST8PDT,M3.2.0/2,M11.1.0/2";
  }

  return tz;
}

static bool parse_offset_hhmm(const String &value, long &seconds_out) {
  String s = value;
  s.trim();
  if (s.length() == 0) {
    return false;
  }

  int sign = 1;
  if (s[0] == '+') {
    s = s.substring(1);
  } else if (s[0] == '-') {
    sign = -1;
    s = s.substring(1);
  }

  int hh = 0;
  int mm = 0;
  if (s.indexOf(':') >= 0) {
    if (sscanf(s.c_str(), "%d:%d", &hh, &mm) < 1) {
      return false;
    }
  } else {
    if (sscanf(s.c_str(), "%d", &hh) != 1) {
      return false;
    }
  }

  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) {
    return false;
  }

  seconds_out = sign * (hh * 3600L + mm * 60L);
  return true;
}

static long resolve_tz_offset_seconds(const String &tz_raw) {
  String tz = to_lower_copy(tz_raw);
  tz.trim();
  if (tz.length() == 0) {
    return 0;
  }

  if (tz == "asia/kolkata" || tz == "asia/calcutta" || tz == "india" || tz == "ist" ||
      tz == "ist-5:30") {
    return 19800;
  }

  if (tz == "utc" || tz == "etc/utc" || tz == "gmt" || tz == "utc0") {
    return 0;
  }

  if (tz.startsWith("utc") || tz.startsWith("gmt")) {
    String tail = tz.substring(3);
    long off = 0;
    if (parse_offset_hhmm(tail, off)) {
      return off;
    }
  }

  // POSIX-style explicit fallback: "XXX-5:30" means UTC+5:30.
  int plus_pos = tz.indexOf('+');
  int minus_pos = tz.indexOf('-');
  int pos = -1;
  if (plus_pos > 0) pos = plus_pos;
  if (minus_pos > 0 && (pos < 0 || minus_pos < pos)) pos = minus_pos;
  if (pos > 0) {
    String tail = tz.substring(pos);
    long off = 0;
    if (parse_offset_hhmm(tail, off)) {
      const char sign = tz[pos];
      if (sign == '-') {
        // POSIX TZ has reversed sign.
        off = -off;
      }
      return off;
    }
  }

  return 0;
}

static bool parse_hhmm(const String &value, int &hour_out, int &minute_out) {
  int hh = -1;
  int mm = -1;
  if (sscanf(value.c_str(), "%d:%d", &hh, &mm) != 2) {
    return false;
  }
  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) {
    return false;
  }
  hour_out = hh;
  minute_out = mm;
  return true;
}

static void ensure_time_configured() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  String tz = String(TIMEZONE_TZ);
  String stored_tz;
  String err;
  if (persona_get_timezone(stored_tz, err)) {
    stored_tz.trim();
    if (stored_tz.length() > 0) {
      tz = stored_tz;
    }
  }
  tz = normalize_tz_for_esp(tz);
  const long offset = resolve_tz_offset_seconds(tz);

  if (s_time_configured && tz == s_last_tz && offset == s_last_tz_offset_seconds) {
    return;
  }

  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
  s_time_configured = true;
  s_last_tz = tz;
  s_last_tz_offset_seconds = offset;
  Serial.println("[scheduler] time sync configured: " + tz + " offset=" + String((long)offset));
}

static bool get_local_time_snapshot(struct tm &tm_out) {
  ensure_time_configured();
  time_t now = time(nullptr);
  if (now < 1700000000) {
    return false;
  }
  now += s_last_tz_offset_seconds;
  gmtime_r(&now, &tm_out);
  return true;
}

static void log_reminder_reason_once(int reason, const String &detail) {
  if (reason == s_last_reminder_reason) {
    return;
  }
  s_last_reminder_reason = reason;
  event_log_append("SCHED reminder: " + detail);
}

void scheduler_init() {
  if (!AUTONOMOUS_STATUS_ENABLED) {
    Serial.println("[scheduler] autonomous status disabled");
  } else {
    s_next_status_ms = millis() + AUTONOMOUS_STATUS_MS;
    Serial.println("[scheduler] autonomous status enabled");
  }

  if (!HEARTBEAT_ENABLED) {
    Serial.println("[scheduler] heartbeat disabled");
  } else {
    s_next_heartbeat_ms = millis() + HEARTBEAT_INTERVAL_MS;
    Serial.println("[scheduler] heartbeat enabled");
  }

  s_next_reminder_check_ms = millis() + 5000;
  Serial.println("[scheduler] daily reminder enabled");

  s_next_cron_check_ms = millis() + 5000;
  Serial.println("[scheduler] cron jobs enabled");
}

void scheduler_tick(incoming_cb_t dispatch_cb) {
  if (dispatch_cb == nullptr) {
    return;
  }

  const unsigned long now = millis();

  if (AUTONOMOUS_STATUS_ENABLED && (long)(now - s_next_status_ms) >= 0) {
    event_log_append("SCHED: status");
    dispatch_cb(String("status"));
    s_next_status_ms = now + AUTONOMOUS_STATUS_MS;
  }

  if (HEARTBEAT_ENABLED && (long)(now - s_next_heartbeat_ms) >= 0) {
    String heartbeat;
    String err;
    if (persona_get_heartbeat(heartbeat, err)) {
      heartbeat.trim();
      if (heartbeat.length() > 0) {
        event_log_append("SCHED: heartbeat_run");
        dispatch_cb(String("heartbeat_run"));
      }
    }
    s_next_heartbeat_ms = now + HEARTBEAT_INTERVAL_MS;
  }

  if ((long)(now - s_next_reminder_check_ms) >= 0) {
    s_next_reminder_check_ms = now + 15000;

    String hhmm;
    String message;
    String err;
    if (!persona_get_daily_reminder(hhmm, message, err)) {
      return;
    }
    hhmm.trim();
    message.trim();
    if (hhmm.length() == 0 || message.length() == 0) {
      log_reminder_reason_once(0, "empty");
      return;
    }

    int target_h = -1;
    int target_m = -1;
    if (!parse_hhmm(hhmm, target_h, target_m)) {
      log_reminder_reason_once(1, "invalid hhmm=" + hhmm);
      return;
    }
    const int target_minute = target_h * 60 + target_m;

    struct tm tm_now{};
    if (!get_local_time_snapshot(tm_now)) {
      log_reminder_reason_once(2, "no time sync");
      return;
    }
    const int now_minute = tm_now.tm_hour * 60 + tm_now.tm_min;

    if (now_minute < target_minute) {
      log_reminder_reason_once(3, "before target now=" + String(tm_now.tm_hour) + ":" +
                                      String(tm_now.tm_min) + " target=" + hhmm);
      return;
    }

    const int late_by = now_minute - target_minute;
    if (late_by > REMINDER_GRACE_MINUTES) {
      log_reminder_reason_once(4, "too late by " + String(late_by) + "m target=" + hhmm);
      return;
    }

    if (tm_now.tm_yday == s_last_reminder_yday &&
        target_minute == s_last_reminder_target_minute) {
      log_reminder_reason_once(5, "already sent today target=" + hhmm);
      return;
    }

    dispatch_cb(String("reminder_run"));
    event_log_append("SCHED: reminder_run target=" + hhmm + " now=" +
                     String(tm_now.tm_hour) + ":" + String(tm_now.tm_min));
    s_last_reminder_yday = tm_now.tm_yday;
    s_last_reminder_target_minute = target_minute;
    s_last_reminder_reason = -1;
  }

  // Cron job checking
  if ((long)(now - s_next_cron_check_ms) >= 0) {
    s_next_cron_check_ms = now + 15000;

    struct tm tm_now{};
    if (!get_local_time_snapshot(tm_now)) {
      // No time sync yet, skip cron check
      return;
    }

    const int current_minute = tm_now.tm_hour * 60 + tm_now.tm_min;

    // Only check once per minute to avoid duplicate triggers
    if (current_minute != s_last_cron_minute) {
      s_last_cron_minute = current_minute;

      CronJob jobs[CRON_MAX_JOBS];
      int job_count = cron_store_get_all(jobs, CRON_MAX_JOBS);

      for (int i = 0; i < job_count; i++) {
        const CronJob &job = jobs[i];

        if (cron_should_trigger(job, tm_now.tm_hour, tm_now.tm_min,
                               tm_now.tm_mday, tm_now.tm_mon + 1, tm_now.tm_wday)) {
          String cmd = job.command;
          cmd.trim();

          event_log_append("SCHED: cron triggered: " + cmd);
          dispatch_cb(cmd);
          Serial.printf("[scheduler] Cron job triggered: %s\n", cmd.c_str());
        }
      }
    }
  }
}

void scheduler_time_debug(String &out) {
  ensure_time_configured();
  time_t now = time(nullptr);

  out = "Time:\n";
  out += "tz_active=" + s_last_tz + "\n";
  out += "tz_offset_sec=" + String((long)s_last_tz_offset_seconds) + "\n";
  out += "epoch=" + String((long)now) + "\n";
  out += "synced=" + String(now >= 1700000000 ? "yes" : "no");

  if (now >= 1700000000) {
    struct tm tm_now{};
    time_t local_now = now + s_last_tz_offset_seconds;
    gmtime_r(&local_now, &tm_now);
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
             tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    out += "\nlocal=" + String(buf);
  }
}
