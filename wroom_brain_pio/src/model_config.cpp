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

// Value suffixes
const char *kApiKeySuffix = "key";
const char *kModelSuffix = "model";

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
  if (lc != "openai" && lc != "anthropic" && lc != "gemini" && lc != "glm" && lc != "none") {
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm, none";
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
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm";
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
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm";
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
  const char *providers[] = {"openai", "anthropic", "gemini", "glm"};

  for (size_t i = 0; i < 4; i++) {
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
  const char *providers[] = {"openai", "anthropic", "gemini", "glm"};

  for (size_t i = 0; i < 4; i++) {
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
    error_out = "Invalid provider. Use: openai, anthropic, gemini, glm";
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
