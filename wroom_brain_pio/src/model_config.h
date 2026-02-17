#ifndef MODEL_CONFIG_H
#define MODEL_CONFIG_H

#include <Arduino.h>

void model_config_init();
String model_config_get_active_provider();
bool model_config_set_active_provider(const String &provider, String &error_out);
String model_config_get_api_key(const String &provider);
bool model_config_set_api_key(const String &provider, const String &key, String &error_out);
String model_config_get_model(const String &provider);
bool model_config_set_model(const String &provider, const String &model, String &error_out);
bool model_config_is_provider_configured(const String &provider);
String model_config_get_configured_list();
String model_config_get_status_summary();
bool model_config_clear_provider(const String &provider, String &error_out);

// Get active configuration with all details
struct ModelConfigInfo {
  String provider;
  String apiKey;
  String model;
  String baseUrl;
};

bool model_config_get_active_config(ModelConfigInfo &config);

// Fallback provider support for quota/rate limit handling
String model_config_get_fallback_provider(const String &exclude_provider);
bool model_config_is_provider_failed(const String &provider);
void model_config_mark_provider_failed(const String &provider, int http_status);
void model_config_reset_failed_provider(const String &provider);
void model_config_reset_all_failed_providers();
String model_config_get_failed_status();

// Timeout for retrying failed providers (milliseconds)
#ifndef MODEL_FAIL_RETRY_MS
#define MODEL_FAIL_RETRY_MS 900000  // 15 minutes
#endif

#endif
