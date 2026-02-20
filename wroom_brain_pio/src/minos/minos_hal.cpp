#include "minos.h"

void hw_init(void) {
    Serial.println("[MinOS] HAL initialized.");
}

void hw_print(const String &s) {
    Serial.print(s);
}

void hw_println(const String &s) {
    Serial.println(s);
}
