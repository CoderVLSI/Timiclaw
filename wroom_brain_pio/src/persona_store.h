#ifndef PERSONA_STORE_H
#define PERSONA_STORE_H

#include <Arduino.h>

void persona_init();

bool persona_get_soul(String &soul_out, String &error_out);
bool persona_set_soul(const String &soul, String &error_out);
bool persona_clear_soul(String &error_out);

bool persona_get_heartbeat(String &heartbeat_out, String &error_out);
bool persona_set_heartbeat(const String &heartbeat, String &error_out);
bool persona_clear_heartbeat(String &error_out);

bool persona_set_daily_reminder(const String &hhmm, const String &message, String &error_out);
bool persona_get_daily_reminder(String &hhmm_out, String &message_out, String &error_out);
bool persona_clear_daily_reminder(String &error_out);

bool persona_set_timezone(const String &tz, String &error_out);
bool persona_get_timezone(String &tz_out, String &error_out);
bool persona_clear_timezone(String &error_out);

bool persona_set_safe_mode(bool enabled, String &error_out);
bool persona_get_safe_mode(bool &enabled_out, String &error_out);

bool persona_set_email_draft(const String &to, const String &subject, const String &body,
                             String &error_out);
bool persona_get_email_draft(String &to_out, String &subject_out, String &body_out,
                             String &error_out);
bool persona_clear_email_draft(String &error_out);

#endif
