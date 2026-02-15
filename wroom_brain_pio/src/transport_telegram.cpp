#include "transport_telegram.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "brain_config.h"

static unsigned long s_last_poll_ms = 0;
static long s_last_update_id = 0;

static String s_last_chat_id = TELEGRAM_ALLOWED_CHAT_ID;

static String url_encode(const String &src) {
  static const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(src.length() * 3);
  for (size_t i = 0; i < src.length(); i++) {
    const unsigned char c = (unsigned char)src[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else if (c == ' ') {
      out += "%20";
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

static String https_get(const String &url, int *status_code) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    if (status_code) {
      *status_code = -1;
    }
    return String();
  }

  const int code = https.GET();
  if (status_code) {
    *status_code = code;
  }

  String body;
  if (code > 0) {
    body = https.getString();
  }

  https.end();
  return body;
}

static int https_post_raw(const String &url, const String &content_type, const String &body,
                          String *response_out) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    return -1;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(20000);
  if (content_type.length() > 0) {
    https.addHeader("Content-Type", content_type);
  }

  const int code = https.POST((uint8_t *)body.c_str(), body.length());
  if (response_out != nullptr) {
    if (code > 0) {
      *response_out = https.getString();
    } else {
      *response_out = "";
    }
  }
  https.end();
  return code;
}

static bool extract_int_after_key(const String &body, const char *key, long *value_out) {
  const int k = body.indexOf(key);
  if (k < 0) {
    return false;
  }

  int i = k + (int)strlen(key);
  while (i < (int)body.length() && (body[i] == ' ' || body[i] == ':')) {
    i++;
  }

  bool neg = false;
  if (i < (int)body.length() && body[i] == '-') {
    neg = true;
    i++;
  }

  long v = 0;
  bool any = false;
  while (i < (int)body.length() && body[i] >= '0' && body[i] <= '9') {
    any = true;
    v = v * 10 + (body[i] - '0');
    i++;
  }

  if (!any) {
    return false;
  }

  *value_out = neg ? -v : v;
  return true;
}

static bool extract_text_field(const String &body, String &text_out) {
  const int key = body.indexOf("\"text\":\"");
  if (key < 0) {
    return false;
  }

  const int start = key + 8;
  String text;
  text.reserve(128);

  bool esc = false;
  for (int i = start; i < (int)body.length(); i++) {
    const char c = body[i];

    if (esc) {
      if (c == 'n') text += '\n';
      else if (c == 't') text += '\t';
      else text += c;
      esc = false;
      continue;
    }

    if (c == '\\') {
      esc = true;
      continue;
    }

    if (c == '"') {
      text_out = text;
      return true;
    }

    text += c;
  }

  return false;
}

static bool extract_chat_id(const String &body, String &chat_id_out) {
  long id = 0;
  if (!extract_int_after_key(body, "\"chat\":{\"id\"", &id)) {
    if (!extract_int_after_key(body, "\"chat\": {\"id\"", &id)) {
      if (!extract_int_after_key(body, "\"chat\":{ \"id\"", &id)) {
        return false;
      }
    }
  }
  chat_id_out = String(id);
  return true;
}

static bool is_wifi_ready() {
  return WiFi.status() == WL_CONNECTED;
}

static void ensure_wifi() {
  if (is_wifi_ready()) {
    return;
  }

  Serial.print("[tg] WiFi connect: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000UL) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[tg] WiFi connected, IP=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[tg] WiFi connect timeout");
  }
}

void transport_telegram_init() {
  ensure_wifi();
  Serial.println("[tg] transport initialized");
}

void transport_telegram_send(const String &msg) {
  if (!is_wifi_ready()) {
    ensure_wifi();
    if (!is_wifi_ready()) {
      return;
    }
  }

  const String text = url_encode(msg);
  const String url = String("https://api.telegram.org/bot") + TELEGRAM_BOT_TOKEN +
                     "/sendMessage?chat_id=" + s_last_chat_id +
                     "&text=" + text;

  int code = 0;
  (void)https_get(url, &code);
  Serial.print("[tg] send code=");
  Serial.println(code);
}

bool transport_telegram_send_document(const String &filename, const String &content,
                                      const String &mime_type, const String &caption) {
  if (!is_wifi_ready()) {
    ensure_wifi();
    if (!is_wifi_ready()) {
      return false;
    }
  }

  String safe_name = filename;
  safe_name.trim();
  if (safe_name.length() == 0) {
    safe_name = "file.txt";
  }

  String safe_mime = mime_type;
  safe_mime.trim();
  if (safe_mime.length() == 0) {
    safe_mime = "text/plain";
  }

  String boundary = "----esp32boundary" + String((unsigned long)millis());
  String payload;
  payload.reserve(content.length() + caption.length() + 512);

  payload += "--" + boundary + "\r\n";
  payload += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
  payload += s_last_chat_id + "\r\n";

  if (caption.length() > 0) {
    payload += "--" + boundary + "\r\n";
    payload += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
    payload += caption + "\r\n";
  }

  payload += "--" + boundary + "\r\n";
  payload += "Content-Disposition: form-data; name=\"document\"; filename=\"" + safe_name + "\"\r\n";
  payload += "Content-Type: " + safe_mime + "\r\n\r\n";
  payload += content;
  payload += "\r\n--" + boundary + "--\r\n";

  const String url = String("https://api.telegram.org/bot") + TELEGRAM_BOT_TOKEN + "/sendDocument";
  String response;
  const int code = https_post_raw(url, "multipart/form-data; boundary=" + boundary, payload, &response);

  Serial.print("[tg] sendDocument code=");
  Serial.println(code);
  return code >= 200 && code < 300;
}

void transport_telegram_poll(incoming_cb_t cb) {
  if (cb == nullptr) {
    return;
  }

  if ((millis() - s_last_poll_ms) < TELEGRAM_POLL_MS) {
    return;
  }
  s_last_poll_ms = millis();

  if (!is_wifi_ready()) {
    ensure_wifi();
    return;
  }

  const String url = String("https://api.telegram.org/bot") + TELEGRAM_BOT_TOKEN +
                     "/getUpdates?timeout=0&limit=1&offset=" + String(s_last_update_id + 1);

  int code = 0;
  const String body = https_get(url, &code);
  if (code != 200 || body.length() == 0) {
    return;
  }

  long update_id = 0;
  if (!extract_int_after_key(body, "\"update_id\"", &update_id)) {
    return;
  }
  if (update_id <= s_last_update_id) {
    return;
  }

  String chat_id;
  if (!extract_chat_id(body, chat_id)) {
    return;
  }

  if (chat_id != String(TELEGRAM_ALLOWED_CHAT_ID)) {
    Serial.println("[tg] rejected message from non-allowlisted chat");
    s_last_update_id = update_id;
    return;
  }

  String text;
  if (!extract_text_field(body, text)) {
    s_last_update_id = update_id;
    return;
  }

  s_last_chat_id = chat_id;
  s_last_update_id = update_id;
  cb(text);
}
