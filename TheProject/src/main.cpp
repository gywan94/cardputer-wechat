#include "MyOS.h"

void setup() {
    //Serial.begin(115200);
    os.begin();
}

void loop() {
    os.loop();
}