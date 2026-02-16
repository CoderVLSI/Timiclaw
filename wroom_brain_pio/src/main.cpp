#include <Arduino.h>
#include <ArduinoOTA.h>
#include "agent_loop.h"
#include "brain_config.h"

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\\n[wroom_brain_pio] boot");
  agent_loop_init();

  // Setup OTA updates
  ArduinoOTA.setHostname("wroom-brain");
  ArduinoOTA.setPassword((const char *)WIFI_PASS);  // Use WiFi password as OTA password

  ArduinoOTA.onStart([]() {
    Serial.println("[ota] Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[ota] Update complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[ota] Progress: %u%%\\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[ota] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("[ota] Ready");
  Serial.print("[ota] IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();
  agent_loop_tick();
  delay(50);
}
