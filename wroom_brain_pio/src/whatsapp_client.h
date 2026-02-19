#ifndef WHATSAPP_CLIENT_H
#define WHATSAPP_CLIENT_H

#include <Arduino.h>

// Send a text message via WhatsApp
// Returns true on success, false on error
bool whatsapp_send_message(const String &message, String &error_out);

// Send web files (HTML, CSS, JS) via WhatsApp
// The service will handle sending the files
bool whatsapp_send_web_files(const String &topic, const String &html, const String &css,
                             const String &js, String &error_out);

#endif
