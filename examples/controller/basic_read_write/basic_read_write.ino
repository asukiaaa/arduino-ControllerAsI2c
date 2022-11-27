#include <Wire.h>

#include <ControllerAsI2c_asukiaaa.hpp>

ControllerAsI2c_asukiaaa::Driver controllerDriver(&Wire);

void setup() {
  Serial.begin(115200);
  controllerDriver.begin();
}

void loop() {
  controllerDriver.read();
  auto info = controllerDriver.getInfo();
  info.print(&Serial);
  delay(500);
}
