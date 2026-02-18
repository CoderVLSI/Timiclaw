#include "model_config.h"

#include <Arduino.h>
#include <Preferences.h>

#include "brain_config.h"

namespace {

Preferences g_prefs;
bool g_ready = false;
const char *kNamespace = "brainmodels";

const char *kActiveProviderKey = "provider_active";

// Provider key prefixes
const char *kOpenaiKeyPrefix = "openai_";
const char *kAnthropicKeyPrefix = "anthropic_";
const char *kGeminiKeyPrefix = "gemini_";
const char *kGlmKeyPrefix = "glm_";
const char *kOpenRouterKeyPrefix = "or_";
const char *kOllamaKeyPrefix = "ollama_";

// Value suffixes
const char *kApiKeySuffix = "key";
const char *kModelSuffix = "model";

// Failed provider tracking suffixes
const char *kFailedTimeSuffix = "_failed";
const char *kFailedStatusSuffix = "_status";

// Provider priority for fallback (in order)
const char *kProviderPriority[] = {"gemini", "openai", "anthropic", "glm", "openrouter", "ollama"};
const size_t kProviderPriorityCount = 6;

String to_lower(String value) {
  value.toLowerCase();
  return value;
}

String make_key(const String &provider_prefix, const char *suffix) {
  return provider_prefix + suffix;
}

String get_provider_prefix(const String &provider) {
  String lc = to_lower(provider);
  if (lc == "openai") {
    return kOpenaiKeyPrefix;
  }
  if (lc == "anthropic") {
    return kAnthropicKeyPrefix;
  }
  if (lc == "gemini") {
    return kGeminiKeyPrefix;
  }
  if (lc == "glm") {
    return kGlmKeyPrefix;
  }
  if (lc == "openrouter" || lc == "openrouter.ai") {
    return kOpenRouterKeyPrefix;
  }
  if (lc == "ollama") {
    return kOllamaKeyPrefix;
  }
  return "";
}

String get_provider_base_url(const String &provider) {
  String lc = to_lower(provider);
  if (lc == "openai") {
    return String(LLM_OPENAI_BASE_URL);
  }
  if (lc == "anthropic") {
    return String(LLM_ANTHROPIC_BASE_URL);
  }
  if (lc == "gemini") {
    return String(LLM_GEMINI_BASE_URL);
  }
  if (lc == "glm") {
    return String(LLM_GLM_BASE_URL);
  }
  if (lc == "openrouter" || lc == "openrouter.ai") {
    return "https://openrouter.ai/api";
  }
  if (lc == "ollama") {
    // Default Ollama endpoint - user can configure their local IP
    return "http://ollama.local:11434/api/generate";
  }
  return "";
}

String get_default_model(const String &provider) {
  String lc = to_lower(provider);
  if (lc == "openai") {
    return "gpt-4.1-mini";
  }
  if (lc == "anthropic") {
    return "claude-3-5-sonnet-latest";
  }
  if (lc == "gemini") {
    return "gemini-2.0-flash";
  }
  if (lc == "glm") {
    return "glm-4.7";
  }
  if (lc == "openrouter" || lc == "openrouter.ai") {
    return "qwen/qwen-2.5-coder-32b-instruct:free";  // Strong free model
  }
  if (lc == "ollama") {
    return "llama3";  // Most common local model
  }
  return "";
}

bool ensure_ready(String &error_out) {
  if (g_ready) {
    return true;
  }
  if (!g_prefs.begin(kNamespace, false)) {
    error_out = "NVS begin failed";
    return false;
  }
  g_ready = true;
  return true;
}

}  // namespace

void model_config_init() {
  String err;
  if (ensure_ready(err)) {
    // Initialize active provider from .env if not set
    String active = g_prefs.getString(kActiveProviderKey, "");
    if (active.length() == 0) {
      String env_provider = String(LLM_PROVIDER);
      env_provider.trim();
      if (env_provider.length() > 0 && env_provider != "none") {
        g_prefs.putString(kActiveProviderKey, env_provider);
        Serial.printf("[model_config] Initialized active provider from .env: %s\n",
                      env_provider.c_str());
      }
    } else {
      Serial.printf("[model_config] Active provider: %s\n", active.c_str());
    }
  } else {
    Serial.println("[model_config] init failed");
  }
}

String model_config_get_active_provider() {
  String err;
  if (!ensure_ready(err)) {
    return "";
  }
  String provider = g_prefs.getString(kActiveProviderKey, "");
  if (provider.length() == 0) {
    // Fallback to .env
    provider = String(LLM_PROVIDER);
  }
  return provider;
}

bool model_config_set_active_provider(const String &provider, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String lc = to_lower(provider);
  if (lc != "openai" && lc != "anthropic" && lc != "gemini" && lc != "glm" && lc != "none" &&
      lc != "openrouter" && lc != "openrouter.ai" && lc != "ollama") {
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm, openrouter, ollama, none";
    return false;
  }

  size_t written = g_prefs.putString(kActiveProviderKey, lc);
  if (written == 0 && lc.length() > 0) {
    error_out = "Failed to write active provider";
    return false;
  }

  Serial.printf("[model_config] Active provider set to: %s\n", lc.c_str());
  return true;
}

String model_config_get_api_key(const String &provider) {
  String err;
  if (!ensure_ready(err)) {
    return "";
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    return "";
  }

  String key = g_prefs.getString(make_key(prefix, kApiKeySuffix).c_str(), "");
  if (key.length() == 0) {
    // Fallback to .env (only for the matching provider)
    String lc = to_lower(provider);
    String env_provider = to_lower(String(LLM_PROVIDER));
    if (lc == env_provider) {
      key = String(LLM_API_KEY);
    }
  }
  return key;
}

bool model_config_set_api_key(const String &provider, const String &key, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm, openrouter, ollama";
    return false;
  }

  String clean_key = key;
  clean_key.trim();
  if (clean_key.length() == 0) {
    error_out = "API key cannot be empty";
    return false;
  }

  size_t written = g_prefs.putString(make_key(prefix, kApiKeySuffix).c_str(), clean_key);
  if (written == 0) {
    error_out = "Failed to write API key";
    return false;
  }

  Serial.printf("[model_config] API key saved for: %s\n", provider.c_str());
  return true;
}

String model_config_get_model(const String &provider) {
  String err;
  if (!ensure_ready(err)) {
    return "";
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    return "";
  }

  String model = g_prefs.getString(make_key(prefix, kModelSuffix).c_str(), "");
  if (model.length() == 0) {
    // Check if .env has a model for this provider
    String lc = to_lower(provider);
    String env_provider = to_lower(String(LLM_PROVIDER));
    String env_model = String(LLM_MODEL);
    if (lc == env_provider && env_model.length() > 0) {
      model = env_model;
    } else {
      model = get_default_model(provider);
    }
  }
  return model;
}

bool model_config_set_model(const String &provider, const String &model, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm, openrouter, ollama";
    return false;
  }

  String clean_model = model;
  clean_model.trim();
  if (clean_model.length() == 0) {
    error_out = "Model cannot be empty";
    return false;
  }

  size_t written = g_prefs.putString(make_key(prefix, kModelSuffix).c_str(), clean_model);
  if (written == 0) {
    error_out = "Failed to write model";
    return false;
  }

  Serial.printf("[model_config] Model set for %s: %s\n", provider.c_str(), model.c_str());
  return true;
}

bool model_config_is_provider_configured(const String &provider) {
  String key = model_config_get_api_key(provider);
  return key.length() > 0;
}

String model_config_get_configured_list() {
  String result = "";
  const char *providers[] = {"openai", "anthropic", "gemini", "glm", "openrouter", "ollama"};

  for (size_t i = 0; i < 6; i++) {
    String provider = providers[i];
    if (model_config_is_provider_configured(provider)) {
      if (result.length() > 0) {
        result += ", ";
      }
      result += provider;
    }
  }

  if (result.length() == 0) {
    result = "(none configured)";
  }
  return result;
}

String model_config_get_status_summary() {
  String result = "=== Model Configuration ===\n\n";

  String active = model_config_get_active_provider();
  result += "Active: " + (active.length() > 0 ? active : "(none)") + "\n\n";

  result += "Configured providers:\n";
  const char *providers[] = {"openai", "anthropic", "gemini", "glm", "openrouter", "ollama"};

  for (size_t i = 0; i < 6; i++) {
    String provider = providers[i];
    bool configured = model_config_is_provider_configured(provider);
    String model = model_config_get_model(provider);

    result += "  ";
    if (provider == active) {
      result += "[";
    }
    result += provider;
    if (provider == active) {
      result += "*";
    }
    result += "]";

    if (configured) {
      result += " ✅ (" + model + ")";
    } else {
      result += " ❌ (no key)";
    }
    result += "\n";
  }

  result += "\n* = active provider\n";
  result += "Use: /model use <provider> to switch";
  return result;
}

bool model_config_clear_provider(const String &provider, String &error_out) {
  if (!ensure_ready(error_out)) {
    return false;
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm, openrouter, ollama";
    return false;
  }

  g_prefs.remove(make_key(prefix, kApiKeySuffix).c_str());
  g_prefs.remove(make_key(prefix, kModelSuffix).c_str());

  Serial.printf("[model_config] Cleared configuration for: %s\n", provider.c_str());
  return true;
}

bool model_config_get_active_config(ModelConfigInfo &config) {
  String provider = model_config_get_active_provider();
  provider.trim();
  provider.toLowerCase();

  if (provider.length() == 0 || provider == "none") {
    return false;
  }

  config.provider = provider;
  config.apiKey = model_config_get_api_key(provider);
  config.model = model_config_get_model(provider);
  config.baseUrl = get_provider_base_url(provider);

  return config.apiKey.length() > 0;
}

// Get the next available fallback provider (excluding the specified one)
String model_config_get_fallback_provider(const String &exclude_provider) {
  String exclude_lc = to_lower(exclude_provider);

  for (size_t i = 0; i < kProviderPriorityCount; i++) {
    String provider = kProviderPriority[i];
    if (provider == exclude_lc) {
      continue;  // Skip the excluded provider
    }
    if (!model_config_is_provider_configured(provider)) {
      continue;  // Not configured
    }
    if (model_config_is_provider_failed(provider)) {
      continue;  // Currently failed
    }
    return provider;  // Found a valid fallback
  }

  return "";  // No fallback available
}

// Check if a provider is currently marked as failed
bool model_config_is_provider_failed(const String &provider) {
  String err;
  if (!ensure_ready(err)) {
    return false;
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    return false;
  }

  unsigned long failed_time = g_prefs.getUInt(
      make_key(prefix, kFailedTimeSuffix).c_str(), 0);
  if (failed_time == 0) {
    return false;  // Never failed
  }

  // Check if timeout has expired
  unsigned long now = millis() / 1000;  // Convert to seconds (approximate)
  if (now < failed_time) {
    // millis() wrapped around, reset
    g_prefs.remove(make_key(prefix, kFailedTimeSuffix).c_str());
    return false;
  }

  unsigned long elapsed_sec = now - failed_time;
  unsigned long timeout_sec = MODEL_FAIL_RETRY_MS / 1000;

  if (elapsed_sec >= timeout_sec) {
    // Timeout expired, clear the failed status
    g_prefs.remove(make_key(prefix, kFailedTimeSuffix).c_str());
    g_prefs.remove(make_key(prefix, kFailedStatusSuffix).c_str());
    return false;
  }

  return true;  // Still within timeout period
}

// Mark a provider as failed with timestamp and HTTP status
void model_config_mark_provider_failed(const String &provider, int http_status) {
  String err;
  if (!ensure_ready(err)) {
    return;
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    return;
  }

  // Store timestamp (seconds since boot, approximate)
  unsigned long now = millis() / 1000;
  g_prefs.putUInt(make_key(prefix, kFailedTimeSuffix).c_str(), now);
  g_prefs.putInt(make_key(prefix, kFailedStatusSuffix).c_str(), http_status);

  Serial.printf("[model_config] Marked %s as failed (HTTP %d), will retry in %ld min\n",
                provider.c_str(), http_status, MODEL_FAIL_RETRY_MS / 60000);
}

// Reset failed status for a specific provider
void model_config_reset_failed_provider(const String &provider) {
  String err;
  if (!ensure_ready(err)) {
    return;
  }

  String prefix = get_provider_prefix(provider);
  if (prefix.length() == 0) {
    return;
  }

  g_prefs.remove(make_key(prefix, kFailedTimeSuffix).c_str());
  g_prefs.remove(make_key(prefix, kFailedStatusSuffix).c_str());

  Serial.printf("[model_config] Reset failed status for: %s\n", provider.c_str());
}

// Reset all failed providers
void model_config_reset_all_failed_providers() {
  String err;
  if (!ensure_ready(err)) {
    return;
  }

  for (size_t i = 0; i < kProviderPriorityCount; i++) {
    String provider = kProviderPriority[i];
    String prefix = get_provider_prefix(provider);
    g_prefs.remove(make_key(prefix, kFailedTimeSuffix).c_str());
    g_prefs.remove(make_key(prefix, kFailedStatusSuffix).c_str());
  }

  Serial.println("[model_config] Reset all failed providers");
}

// Get status of failed providers
String model_config_get_failed_status() {
  String result = "";
  bool any_failed = false;

  for (size_t i = 0; i < kProviderPriorityCount; i++) {
    String provider = kProviderPriority[i];
    if (model_config_is_provider_failed(provider)) {
      any_failed = true;
      if (result.length() > 0) {
        result += ", ";
      }
      result += provider;
    }
  }

  if (!any_failed) {
    return "No providers currently failed.";
  }

  return "Failed providers: " + result;
}
