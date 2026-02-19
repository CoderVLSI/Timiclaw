#include "whatsapp_client.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "brain_config.h"

namespace {

String json_escape(const String &src) {
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

String trim_right_slashes(String s) {
  while (s.endsWith("/")) {
    s.remove(s.length() - 1);
  }
  return s;
}

}  // namespace

bool whatsapp_send_message(const String &message, String &error_out) {
  String endpoint = String(WHATSAPP_ENDPOINT_URL);
  endpoint.trim();

  if (endpoint.length() == 0) {
    error_out = "WHATSAPP_ENDPOINT_URL not configured in .env";
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    error_out = "WiFi not connected";
    return false;
  }

  const String url = trim_right_slashes(endpoint) + "/send-message";
  const String api_key = String(WHATSAPP_API_KEY);

  String body = "{";
  body += "\"message\":\"" + json_escape(message) + "\"";
  body += "}";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    error_out = "HTTP begin failed";
    return false;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(WHATSAPP_TIMEOUT_MS);
  https.addHeader("Content-Type", "application/json");

  if (api_key.length() > 0) {
    https.addHeader("Authorization", "Bearer " + api_key);
    https.addHeader("X-API-Key", api_key);
  }

  int code = https.POST((uint8_t *)body.c_str(), body.length());
  String resp_body;
  if (code > 0) {
    resp_body = https.getString();
  }
  https.end();

  if (code < 200 || code >= 300) {
    error_out = "WhatsApp HTTP " + String(code);
    if (resp_body.length() > 0) {
      error_out += ": " + resp_body.substring(0, 100);
    }
    return false;
  }

  return true;
}

bool whatsapp_send_web_files(const String &topic, const String &html, const String &css,
                             const String &js, String &error_out) {
  String endpoint = String(WHATSAPP_ENDPOINT_URL);
  endpoint.trim();

  if (endpoint.length() == 0) {
    error_out = "WHATSAPP_ENDPOINT_URL not configured in .env";
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    error_out = "WiFi not connected";
    return false;
  }

  const String url = trim_right_slashes(endpoint) + "/send-files";
  const String api_key = String(WHATSAPP_API_KEY);

  String body = "{";
  body += "\"topic\":\"" + json_escape(topic) + "\",";
  body += "\"html\":\"" + json_escape(html) + "\",";
  body += "\"css\":\"" + json_escape(css) + "\",";
  body += "\"js\":\"" + json_escape(js) + "\"";
  body += "}";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    error_out = "HTTP begin failed";
    return false;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(WHATSAPP_TIMEOUT_MS);
  https.addHeader("Content-Type", "application/json");

  if (api_key.length() > 0) {
    https.addHeader("Authorization", "Bearer " + api_key);
    https.addHeader("X-API-Key", api_key);
  }

  int code = https.POST((uint8_t *)body.c_str(), body.length());
  String resp_body;
  if (code > 0) {
    resp_body = https.getString();
  }
  https.end();

  if (code < 200 || code >= 300) {
    error_out = "WhatsApp HTTP " + String(code);
    if (resp_body.length() > 0) {
      error_out += ": " + resp_body.substring(0, 100);
    }
    return false;
  }

  return true;
}
