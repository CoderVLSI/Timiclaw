#include <Arduino.h>
#include "agent_loop.h"

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\\n[wroom_brain_pio] boot");
  agent_loop_init();
}

void loop() {
  agent_loop_tick();
  delay(50);
}
