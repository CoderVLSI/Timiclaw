#ifndef CRON_PARSER_H
#define CRON_PARSER_H

#include <Arduino.h>

// Maximum number of cron jobs supported
#ifndef CRON_MAX_JOBS
#define CRON_MAX_JOBS 10
#endif

// Individual cron job definition
struct CronJob {
  int minute;      // 0-59, or -1 for wildcard
  int hour;        // 0-23, or -1 for wildcard
  int day;         // 1-31, or -1 for wildcard
  int month;       // 1-12, or -1 for wildcard
  int weekday;     // 0-6 (Sun=0), or -1 for wildcard
  String command;  // Command to execute (e.g., "reminder_run")

  CronJob() : minute(-1), hour(-1), day(-1), month(-1), weekday(-1), command("") {}
};

// Parse cron expression (5 fields: minute hour day month weekday)
// Returns true on success, false on error
// Example: "0 9 * * * | Morning medicine" -> sets minute=0, hour=9, others=-1
bool cron_parse_line(const String &line, CronJob &job, String &error_out);

// Check if a cron job should trigger at the given time
bool cron_should_trigger(const CronJob &job, int hour, int minute, int day, int month, int weekday);

// Format cron job as string for display
String cron_job_to_string(const CronJob &job);

#endif
