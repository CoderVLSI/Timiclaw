#ifndef DISCORD_CLIENT_H
#define DISCORD_CLIENT_H

#include <Arduino.h>

// Send a plain text message via Discord Webhook
// Returns true on success, false on error
bool discord_send_message(const String &message, String &error_out);

// Send web files (HTML, CSS, JS) via Discord Webhook as attachments
bool discord_send_web_files(const String &topic, const String &html, const String &css,
                             const String &js, String &error_out);

#endif
