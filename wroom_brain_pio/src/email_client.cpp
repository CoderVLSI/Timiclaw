#include "email_client.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "brain_config.h"

namespace {

static String json_escape(const String &src) {
  String out;
  out.reserve(src.length() * 1.2);
  for (size_t i = 0; i < src.length(); i++) {
    const char c = src[i];
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

struct HttpResult {
  int status_code;
  String body;
};

static HttpResult http_post_json(const String &url, const String &json_body,
                                   const String &auth_header = "") {
  HttpResult result;
  result.status_code = -1;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    return result;
  }

  https.addHeader("Content-Type", "application/json");
  if (auth_header.length() > 0) {
    https.addHeader("Authorization", auth_header);
  }

  const int code = https.POST(json_body);
  result.status_code = code;
  result.body = https.getString();

  https.end();
  return result;
}

}  // namespace

static String to_lower(String value) {
  value.toLowerCase();
  return value;
}

bool email_send(const String &to, const String &subject, const String &html_content,
                 const String &text_content, String &error_out) {

  if (to.length() == 0 || subject.length() == 0) {
    error_out = "Missing required fields: to, subject";
    return false;
  }

  if (html_content.length() == 0 && text_content.length() == 0) {
    error_out = "Either html_content or text_content is required";
    return false;
  }

  const String provider = to_lower(String(EMAIL_PROVIDER));

  if (provider == "sendgrid") {
    const String api_key = String(SENDGRID_API_KEY);
    if (api_key.length() == 0) {
      error_out = "Missing SENDGRID_API_KEY in .env";
      return false;
    }

    const String from_email = String(EMAIL_FROM);
    if (from_email.length() == 0) {
      error_out = "Missing EMAIL_FROM in .env";
      return false;
    }

    const String url = "https://api.sendgrid.com/v3/mail/send";

    String body = "{";
    body += "\"personalizations\":[{";
    body += "\"to\":[{\"email\":\"" + json_escape(to) + "\"}]";
    body += "}],";
    body += "\"from\":{\"email\":\"" + json_escape(from_email) + "\"},";
    body += "\"subject\":\"" + json_escape(subject) + "\",";
    body += "\"content\":[";

    bool has_content = false;
    if (text_content.length() > 0) {
      body += "{\"type\":\"text/plain\",\"value\":\"" + json_escape(text_content) + "\"}";
      has_content = true;
    }
    if (html_content.length() > 0) {
      if (has_content) body += ",";
      body += "{\"type\":\"text/html\",\"value\":\"" + json_escape(html_content) + "\"}";
    }

    body += "]}";

    const HttpResult res = http_post_json(url, body, "Bearer " + api_key);

    if (res.status_code < 200 || res.status_code >= 300) {
      error_out = "SendGrid HTTP " + String(res.status_code);
      if (res.body.length() > 0) {
        error_out += ": " + res.body.substring(0, 100);
      }
      return false;
    }

    return true;
  }

  // Default: Resend
  const String api_key = String(RESEND_API_KEY);
  if (api_key.length() == 0) {
    error_out = "Missing RESEND_API_KEY in .env (or use EMAIL_PROVIDER=sendgrid)";
    return false;
  }

  const String from_email = String(EMAIL_FROM);
  if (from_email.length() == 0) {
    error_out = "Missing EMAIL_FROM in .env";
    return false;
  }

  const String url = "https://api.resend.com/emails";

  String body = "{";
  body += "\"from\":\"" + json_escape(from_email) + "\",";
  body += "\"to\":\"" + json_escape(to) + "\",";
  body += "\"subject\":\"" + json_escape(subject) + "\"";

  if (html_content.length() > 0) {
    body += ",\"html\":\"" + json_escape(html_content) + "\"";
  }

  if (text_content.length() > 0) {
    body += ",\"text\":\"" + json_escape(text_content) + "\"";
  }

  body += "}";

  const HttpResult res = http_post_json(url, body, "Bearer " + api_key);

  if (res.status_code < 200 || res.status_code >= 300) {
    error_out = "Resend HTTP " + String(res.status_code);
    if (res.body.length() > 0) {
      error_out += ": " + res.body.substring(0, 100);
    }
    return false;
  }

  return true;
}
