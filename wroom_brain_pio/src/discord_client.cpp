#include "discord_client.h"

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

}  // namespace

bool discord_send_message(const String &message, String &error_out) {
  String webhook_url = String(DISCORD_WEBHOOK_URL);
  webhook_url.trim();

  if (webhook_url.length() == 0) {
    error_out = "DISCORD_WEBHOOK_URL not configured in .env";
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    error_out = "WiFi not connected";
    return false;
  }

  String body = "{ \"content\": \"" + json_escape(message) + "\" }";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, webhook_url)) {
    error_out = "HTTP begin failed";
    return false;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(15000);
  https.addHeader("Content-Type", "application/json");

  int code = https.POST((uint8_t *)body.c_str(), body.length());
  String resp_body;
  if (code > 0) {
    resp_body = https.getString();
  }
  https.end();

  if (code < 200 || code >= 300) {
    error_out = "Discord HTTP " + String(code);
    if (resp_body.length() > 0) {
      error_out += ": " + resp_body.substring(0, 100);
    }
    return false;
  }

  return true;
}

bool discord_send_web_files(const String &topic, const String &html, const String &css,
                             const String &js, String &error_out) {
  String webhook_url = String(DISCORD_WEBHOOK_URL);
  webhook_url.trim();

  if (webhook_url.length() == 0) {
    error_out = "DISCORD_WEBHOOK_URL not configured in .env";
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    error_out = "WiFi not connected";
    return false;
  }

  // Create combined HTML file
  String fullHtml = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
  fullHtml += "  <meta charset=\"UTF-8\">\n";
  fullHtml += "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  fullHtml += "  <title>" + (topic.length() > 0 ? topic : String("Generated Files")) + "</title>\n";
  if (css.length() > 0) fullHtml += "  <style>\n" + css + "\n  </style>\n";
  fullHtml += "</head>\n<body>\n";
  if (html.length() > 0) fullHtml += html + "\n";
  if (js.length() > 0) fullHtml += "<script>\n" + js + "\n</script>\n";
  fullHtml += "</body>\n</html>";

  // Build multipart/form-data payload
  String boundary = "----TimiClawDiscordBoundary";
  String body = "";

  // Title Content
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"content\"\r\n\r\n";
  body += "📄 Generated website files for: **" + (topic.length() > 0 ? topic : String("website")) + "**\r\n";

  // Attached HTML File
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + (topic.length() > 0 ? topic : String("website")) + ".html\"\r\n";
  body += "Content-Type: text/html\r\n\r\n";
  body += fullHtml + "\r\n";
  
  // Close Boundary
  body += "--" + boundary + "--\r\n";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, webhook_url)) {
    error_out = "HTTP begin failed";
    return false;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(25000);
  https.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  int code = https.POST((uint8_t *)body.c_str(), body.length());
  String resp_body;
  if (code > 0) {
    resp_body = https.getString();
  }
  https.end();

  if (code < 200 || code >= 300) {
    error_out = "Discord HTTP " + String(code);
    if (resp_body.length() > 0) {
      error_out += ": " + resp_body.substring(0, 100);
    }
    return false;
  }

  return true;
}
