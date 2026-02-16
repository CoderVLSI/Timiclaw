#ifndef EMAIL_CLIENT_H
#define EMAIL_CLIENT_H

#include <Arduino.h>

bool email_send(const String &to, const String &subject, const String &html_content,
                 const String &text_content, String &error_out);

#endif
