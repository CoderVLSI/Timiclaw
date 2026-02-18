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

#endif
