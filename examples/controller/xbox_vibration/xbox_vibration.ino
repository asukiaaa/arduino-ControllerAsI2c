#include <ControllerAsI2c_asukiaaa.hpp>

ControllerAsI2c_asukiaaa::Driver controllerDriver(&Wire);

void setup() {
  Serial.begin(115200);
  controllerDriver.begin();
}

bool changedToDoublePress = false;

void loop() {
  controllerDriver.read();
  auto info = controllerDriver.getInfo();

  if (info.stateRead != 0) {
    Serial.println("not connected. state:" + String(info.stateRead));
  } else {
    Serial.println("connected.");
    Serial.println("Y and B: all full power 1 second");
    Serial.println("X and A: shake 50\% power 0.5 second");
    Serial.println("A and B: center 100\% power 0.1 second");
    if (info.buttonsDirR[0] && info.buttonsDirR[2]) {
      if (!changedToDoublePress) {
        Serial.println("detect Y and B. wrote vibration");
        changedToDoublePress = true;
        auto writable = controllerDriver.getXboxSeriesXDataWritableP();
        writable->report.setFullPowerFor1Sec();
        controllerDriver.writeToXboxSeriesX();
      }
    } else if (info.buttonsDirR[1] && info.buttonsDirR[3]) {
      if (!changedToDoublePress) {
        Serial.println("detect X and A. wrote vibration");
        changedToDoublePress = true;
        auto writable = controllerDriver.getXboxSeriesXDataWritableP();
        writable->report.setAllOff();
        writable->report.v.select.shake = true;
        writable->report.v.power.shake = 50;
        writable->report.v.timeActive = 50;
        controllerDriver.writeToXboxSeriesX();
      }
    } else if (info.buttonsDirR[2] && info.buttonsDirR[3]) {
      if (!changedToDoublePress) {
        Serial.println("detect A and B. wrote vibration");
        changedToDoublePress = true;
        auto writable = controllerDriver.getXboxSeriesXDataWritableP();
        writable->report.setAllOff();
        writable->report.v.select.center = true;
        writable->report.v.power.center = 100;
        writable->report.v.timeActive = 10;
        controllerDriver.writeToXboxSeriesX();
      }
    } else {
      changedToDoublePress = false;
    }
  }
  delay(500);
}
