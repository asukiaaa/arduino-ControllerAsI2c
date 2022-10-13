#include <CRCx.h>
#include <Wire.h>

#include <ControllerAsI2c_asukiaaa.hpp>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <wire_asukiaaa.hpp>

XboxSeriesXControllerESP32_asukiaaa::Core controller;
wire_asukiaaa::PeripheralHandler peri(&Wire, 0xff);
namespace Register = ControllerAsI2c_asukiaaa::Register;

void setCrc16(uint8_t* dataArr, uint8_t dataLen, uint8_t dataMaxLen) {
  uint16_t crc16 = crcx::crc16(dataArr, dataLen);
  if ((int)dataMaxLen - dataLen < 2) {
    return;
  }
  dataArr[dataLen] = crc16 >> 8;
  dataArr[dataLen + 1] = crc16 & 0xff;
  Serial.println(crc16);
  Serial.println(String(dataArr[dataLen]) + " " + String(dataArr[dataLen + 1]));
}

void taskController(void* param) {
  size_t dataLen = 3 + controller.notifByteLen + 2;
  peri.buffs[Register::dataLen] = dataLen;
  peri.buffs[Register::controllerType] =
      (uint8_t)ControllerAsI2c_asukiaaa::ControllerType::Xbox;
  while (true) {
    controller.onLoop();
    if (controller.isConnected()) {
      peri.buffs[Register::controllerState] =
          (uint8_t)ControllerAsI2c_asukiaaa::ConectionState::connected;
      auto notif = controller.xboxNotif;
      memcpy(&peri.buffs[Register::dataStart], controller.notifByteArr,
             controller.notifByteLen);
      setCrc16(peri.buffs, dataLen, peri.buffLen);
    } else {
      peri.buffs[Register::controllerState] =
          controller.getCountFailedConnection() == 0
              ? (uint8_t)ControllerAsI2c_asukiaaa::ConectionState::notFound
              : (uint8_t)ControllerAsI2c_asukiaaa::ConectionState::
                    foundButNotConnected;
      setCrc16(peri.buffs, dataLen, peri.buffLen);
      if (controller.getCountFailedConnection() > 3) {
        Serial.println("failed connection");
        ESP.restart();
      }
    }
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  controller.begin();
  Wire.onReceive([](int i) { peri.onReceive(i); });
  Wire.onRequest([]() { peri.onRequest(); });
  Wire.begin((uint8_t)CONTROLLER_AS_I2C_TARGET_ADDRESS);
  // loopはcore0で実行されます。xTaskCreateの最後の引数がコア番号を意味します。
  xTaskCreatePinnedToCore(taskController, "taskController", 4096, NULL, 1, NULL,
                          1);
}

void loop() { delay(100); }
