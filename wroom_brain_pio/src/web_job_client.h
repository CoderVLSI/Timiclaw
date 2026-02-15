#ifndef WEB_JOB_CLIENT_H
#define WEB_JOB_CLIENT_H

#include <Arduino.h>

bool web_job_run(const String &task, const String &timezone, String &reply_out, String &error_out);

#endif
