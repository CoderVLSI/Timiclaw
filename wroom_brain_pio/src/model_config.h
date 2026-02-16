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

#endif
