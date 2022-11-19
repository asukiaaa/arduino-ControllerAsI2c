#include <CRCx.h>
#include <Wire.h>

#include <ControllerAsI2c_asukiaaa.hpp>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <wire_asukiaaa.hpp>

// #define TARGET_CONTROLLER_ADDRESS "68:6c:e6:3a:ac:ba"

#ifdef TARGET_CONTROLLER_ADDRESS
XboxSeriesXControllerESP32_asukiaaa::Core controller(TARGET_CONTROLLER_ADDRESS);
#else
XboxSeriesXControllerESP32_asukiaaa::Core controller;
#endif

wire_asukiaaa::PeripheralHandler peri(&Wire, 0xff);
namespace Register = ControllerAsI2c_asukiaaa::Register;

void setCrc16(uint8_t* dataArr, uint8_t dataLen, uint8_t dataMaxLen) {
  uint16_t crc16 = crcx::crc16(dataArr, dataLen);
  if ((int)dataMaxLen - dataLen < 2) {
    return;
  }
  dataArr[dataLen] = crc16 >> 8;
  dataArr[dataLen + 1] = crc16 & 0xff;
}

bool changedToConnected = false;

void taskController(void* param) {
  size_t dataLenWithoutCrc = 3 + controller.notifByteLen;
  size_t dataLen = dataLenWithoutCrc + 2;
  peri.buffs[Register::DataLen] = dataLen;
  peri.buffs[Register::ControllerType] =
      (uint8_t)ControllerAsI2c_asukiaaa::ControllerType::Xbox;
  while (true) {
    controller.onLoop();
    if (controller.isConnected()) {
      if (controller.isWaitingForFirstNotification()) {
        peri.buffs[Register::ConnectionState] =
            (uint8_t)ControllerAsI2c_asukiaaa::ConnectionState::
                WaitingForFirstNotification;
      } else {
        if (!changedToConnected) {
          changedToConnected = true;
          Serial.println("Connected to " + controller.buildDeviceAddressStr());
        }
        peri.buffs[Register::ConnectionState] =
            (uint8_t)ControllerAsI2c_asukiaaa::ConnectionState::Connected;
        auto notif = controller.xboxNotif;
        memcpy(&peri.buffs[Register::DataStart], controller.notifByteArr,
               controller.notifByteLen);
      }
      setCrc16(peri.buffs, dataLenWithoutCrc, peri.buffLen);
    } else {
      changedToConnected = false;
      peri.buffs[Register::ConnectionState] =
          controller.getCountFailedConnection() == 0
              ? (uint8_t)ControllerAsI2c_asukiaaa::ConnectionState::NotFound
              : (uint8_t)ControllerAsI2c_asukiaaa::ConnectionState::
                    FoundButNotConnected;
      setCrc16(peri.buffs, dataLenWithoutCrc, peri.buffLen);
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
  Wire.begin((uint8_t)CONTROLLER_AS_I2C_TARGET_ADDRESS, SDA, SCL, 0);
  // loopはcore0で実行されます。xTaskCreateの最後の引数がコア番号を意味します。
  xTaskCreatePinnedToCore(taskController, "taskController", 4096, NULL, 1, NULL,
                          1);
}

void loop() { delay(100); }
