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
  auto infoReadonly = controllerDriver.getXboxSeriesXDataReadonlyP();
  if (info.stateRead == 0 &&
      info.controllerType ==
          ControllerAsI2c_asukiaaa::Common::ControllerType::XboxSeriesX) {
    Serial.println("xbox battery: " + String(infoReadonly->battery));
  }
  Serial.println();
  delay(500);
}
