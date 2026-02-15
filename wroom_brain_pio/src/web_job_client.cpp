#include "web_job_client.h"

#include <Arduino.h>
#include <cstring>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "brain_config.h"

namespace {

enum SearchProviderMode {
  SEARCH_PROVIDER_AUTO = 0,
  SEARCH_PROVIDER_TAVILY = 1,
  SEARCH_PROVIDER_DDG = 2,
};

String to_lower_copy(String value) {
  value.toLowerCase();
  return value;
}

String trim_right_slashes(String s) {
  while (s.endsWith("/")) {
    s.remove(s.length() - 1);
  }
  return s;
}

String json_escape(const String &src) {
  String out;
  out.reserve(src.length() + 32);
  for (size_t i = 0; i < src.length(); i++) {
    const char c = src[i];
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
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
        if ((unsigned char)c < 0x20) {
          out += ' ';
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

bool extract_json_string_after(const String &body, const char *needle, String &out) {
  const int key = body.indexOf(needle);
  if (key < 0) {
    return false;
  }

  const int start = key + (int)strlen(needle);
  String text;
  text.reserve(256);
  bool esc = false;

  for (int i = start; i < (int)body.length(); i++) {
    const char c = body[i];
    if (esc) {
      switch (c) {
        case 'n':
          text += '\n';
          break;
        case 'r':
          text += '\r';
          break;
        case 't':
          text += '\t';
          break;
        case '\\':
          text += '\\';
          break;
        case '"':
          text += '"';
          break;
        default:
          text += c;
          break;
      }
      esc = false;
      continue;
    }

    if (c == '\\') {
      esc = true;
      continue;
    }
    if (c == '"') {
      out = text;
      return true;
    }
    text += c;
  }

  return false;
}

String summarize_http_body(const String &body) {
  String brief = body;
  brief.replace('\n', ' ');
  brief.replace('\r', ' ');
  brief.trim();
  if (brief.length() > 180) {
    brief = brief.substring(0, 180);
  }
  return brief;
}

SearchProviderMode resolve_provider_mode() {
  String mode = String(WEB_SEARCH_PROVIDER);
  mode.trim();
  mode.toLowerCase();
  if (mode == "tavily") {
    return SEARCH_PROVIDER_TAVILY;
  }
  if (mode == "ddg") {
    return SEARCH_PROVIDER_DDG;
  }
  return SEARCH_PROVIDER_AUTO;
}

String url_encode(const String &value) {
  const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(value.length() * 3);
  for (size_t i = 0; i < value.length(); i++) {
    const unsigned char c = (unsigned char)value[i];
    const bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                      c == '.' || c == '~';
    if (safe) {
      out += (char)c;
    } else if (c == ' ') {
      out += '+';
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

bool duckduckgo_instant_search(const String &query, String &reply_out, String &error_out) {
  const String url = String("https://api.duckduckgo.com/?format=json&no_html=1&skip_disambig=1&q=") +
                     url_encode(query);

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    error_out = "HTTP begin failed";
    return false;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(WEB_SEARCH_TIMEOUT_MS);
  const int code = https.GET();
  String body;
  if (code > 0) {
    body = https.getString();
  }
  https.end();

  if (code < 200 || code >= 300) {
    error_out = "DDG HTTP " + String(code);
    String brief = summarize_http_body(body);
    if (brief.length() > 0) {
      error_out += " " + brief;
    }
    return false;
  }

  String abstract_text;
  String answer;
  String heading;
  String source_url;
  String related;
  extract_json_string_after(body, "\"AbstractText\":\"", abstract_text);
  extract_json_string_after(body, "\"Answer\":\"", answer);
  extract_json_string_after(body, "\"Heading\":\"", heading);
  extract_json_string_after(body, "\"AbstractURL\":\"", source_url);
  extract_json_string_after(body, "\"Text\":\"", related);

  String picked;
  if (abstract_text.length() > 0) {
    picked = abstract_text;
  } else if (answer.length() > 0) {
    picked = answer;
  } else if (related.length() > 0) {
    picked = related;
  }

  picked.trim();
  heading.trim();
  source_url.trim();
  if (picked.length() == 0) {
    error_out = "No quick result.";
    return false;
  }

  reply_out = "Web result (quick):";
  if (heading.length() > 0) {
    reply_out += "\n" + heading;
  }
  reply_out += "\n" + picked;
  if (source_url.length() > 0) {
    reply_out += "\n" + source_url;
  }
  if (reply_out.length() > 1400) {
    reply_out = reply_out.substring(0, 1400);
  }
  return true;
}

bool tavily_search(const String &query, String &reply_out, String &error_out) {
  String api_key = String(WEB_SEARCH_API_KEY);
  api_key.trim();
  if (api_key.length() == 0) {
    error_out = "Missing WEB_SEARCH_API_KEY for Tavily";
    return false;
  }

  String base = String(WEB_SEARCH_BASE_URL);
  base.trim();
  if (base.length() == 0 || base.indexOf("search.brave.com") >= 0) {
    base = "https://api.tavily.com";
  }
  const String url = trim_right_slashes(base) + "/search";

  const int max_results = (WEB_SEARCH_RESULTS_MAX < 1) ? 1 : WEB_SEARCH_RESULTS_MAX;
  const String body = String("{\"api_key\":\"") + json_escape(api_key) +
                      "\",\"query\":\"" + json_escape(query) +
                      "\",\"search_depth\":\"basic\",\"max_results\":" + String(max_results) +
                      ",\"include_answer\":\"basic\"}";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    error_out = "HTTP begin failed";
    return false;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(WEB_SEARCH_TIMEOUT_MS);
  https.addHeader("Content-Type", "application/json");

  const int code = https.POST((uint8_t *)body.c_str(), body.length());
  String resp_body;
  if (code > 0) {
    resp_body = https.getString();
  }
  https.end();

  if (code < 200 || code >= 300) {
    error_out = "Tavily HTTP " + String(code);
    String brief = summarize_http_body(resp_body);
    if (brief.length() > 0) {
      error_out += " " + brief;
    }
    return false;
  }

  String answer;
  String title;
  String source_url;
  String content;
  extract_json_string_after(resp_body, "\"answer\":\"", answer);
  extract_json_string_after(resp_body, "\"title\":\"", title);
  extract_json_string_after(resp_body, "\"url\":\"", source_url);
  extract_json_string_after(resp_body, "\"content\":\"", content);

  answer.trim();
  title.trim();
  source_url.trim();
  content.trim();

  if (answer.length() == 0 && content.length() == 0 && title.length() == 0) {
    error_out = "Tavily returned no useful result";
    return false;
  }

  reply_out = "Web result:";
  if (answer.length() > 0) {
    reply_out += "\n" + answer;
  }
  if (title.length() > 0) {
    reply_out += "\nSource: " + title;
  }
  if (source_url.length() > 0) {
    reply_out += "\n" + source_url;
  }
  if (answer.length() == 0 && content.length() > 0) {
    reply_out += "\n" + content;
  }
  if (reply_out.length() > 1400) {
    reply_out = reply_out.substring(0, 1400);
  }
  return true;
}

}  // namespace

bool web_job_run(const String &task, const String &timezone, String &reply_out, String &error_out) {
  (void)timezone;

  if (WiFi.status() != WL_CONNECTED) {
    error_out = "WiFi not connected";
    return false;
  }

  String endpoint = String(WEB_JOB_ENDPOINT_URL);
  endpoint.trim();
  if (endpoint.length() == 0) {
    const SearchProviderMode mode = resolve_provider_mode();
    if (mode == SEARCH_PROVIDER_TAVILY || mode == SEARCH_PROVIDER_AUTO) {
      String tavily_err;
      if (tavily_search(task, reply_out, tavily_err)) {
        return true;
      }
      if (mode == SEARCH_PROVIDER_TAVILY) {
        error_out = tavily_err;
        return false;
      }
    }

    String ddg_err;
    if (duckduckgo_instant_search(task, reply_out, ddg_err)) {
      return true;
    }
    error_out = ddg_err;
    return false;
  }

  const String url = trim_right_slashes(endpoint) + "/run_job";
  const String api_key = String(WEB_JOB_API_KEY);

  String body = String("{\"task\":\"") + json_escape(task) + "\",\"timezone\":\"" +
                json_escape(timezone) + "\",\"device\":\"esp32\"}";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (!https.begin(client, url)) {
    error_out = "HTTP begin failed";
    return false;
  }

  https.setConnectTimeout(12000);
  https.setTimeout(WEB_JOB_TIMEOUT_MS);
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
    error_out = "WEBJOB HTTP " + String(code);
    String brief = summarize_http_body(resp_body);
    if (brief.length() > 0) {
      error_out += " " + brief;
    }
    return false;
  }

  if (extract_json_string_after(resp_body, "\"reply\":\"", reply_out) ||
      extract_json_string_after(resp_body, "\"result\":\"", reply_out) ||
      extract_json_string_after(resp_body, "\"text\":\"", reply_out) ||
      extract_json_string_after(resp_body, "\"content\":\"", reply_out)) {
    reply_out.trim();
    if (reply_out.length() > 0) {
      return true;
    }
  }

  reply_out = summarize_http_body(resp_body);
  if (reply_out.length() == 0) {
    error_out = "empty response";
    return false;
  }
  return true;
}
