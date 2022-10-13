#include <Wire.h>

#include <ControllerAsI2c_asukiaaa.hpp>

ControllerAsI2c_asukiaaa::Driver<TwoWire> controllerDriver(&Wire);
ControllerAsI2c_asukiaaa::Info controllerInfo;

void setup() {
  Serial.begin(115200);
  controllerDriver.begin();
}

void loop() {
  controllerDriver.read(&controllerInfo);
  controllerInfo.print(&Serial);
  delay(2000);
}
