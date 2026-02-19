#ifndef CRON_STORE_H
#define CRON_STORE_H

#include <Arduino.h>
#include "cron_parser.h"

// Initialize cron store (loads cron.md if exists)
void cron_store_init();

// Add a cron job to cron.md
bool cron_store_add(const String &cron_line, String &error_out);

// Get all cron jobs
// Returns number of jobs loaded
int cron_store_get_all(CronJob *jobs, int max_jobs);

// Clear all cron jobs from cron.md
bool cron_store_clear(String &error_out);

// Get cron.md file content as string
bool cron_store_get_content(String &content_out, String &error_out);

// Get count of active cron jobs
int cron_store_count();

// ============================================================================
// MISSED JOB TRACKING
// ============================================================================

// Struct to represent a missed job
struct MissedJob {
  String command;
  int missed_hour;
  int missed_minute;
  int missed_day;
  int missed_month;
  int missed_weekday;
};

// Get the last successful cron check timestamp (epoch seconds)
// Returns 0 if never checked
time_t cron_store_get_last_check();

// Update the last successful cron check timestamp
void cron_store_update_last_check(time_t timestamp);

// Check for missed jobs between last_check and now
// Returns number of missed jobs (max 10)
// Fills missed_jobs array with details
int cron_store_check_missed_jobs(time_t now, MissedJob *missed_jobs, int max_jobs);

#endif
