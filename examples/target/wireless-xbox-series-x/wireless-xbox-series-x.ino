#include <CRCx.h>
#include <Wire.h>
#include <button_asukiaaa.h>
#include <string_asukiaaa.h>

#include <ControllerAsI2c_asukiaaa.hpp>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <wire_asukiaaa.hpp>

#define PIN_BTN_TEST 0
// #define PIN_BTN_TEST 39 // m5stack btnA

// #define TEST_VIBRATION

button_asukiaaa::Button btnTest(PIN_BTN_TEST);

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

void convertToDataArr(String dataStr, uint8_t* dataArr, size_t dataLen) {
  String formattedStr = dataStr;
  formattedStr.replace(" ", "");
  formattedStr = string_asukiaaa::padStart(formattedStr, 2 * dataLen, '0');
  auto len = formattedStr.length();
  // Serial.println(formattedStr);
  // Serial.println(len);
  formattedStr = formattedStr.substring(len - dataLen * 2);
  // Serial.println(formattedStr);
  for (int i = 0; i < dataLen; ++i) {
    String strHex =
        String(formattedStr[i * 2]) + String(formattedStr[i * 2 + 1]);
    auto v = strtol(strHex.c_str(), 0, 16);
    // Serial.println(strHex + " -> " + String(v));
    dataArr[dataLen - i - 1] = v;
  }
}

void taskController(void* param) {
  size_t dataLenWithoutCrc = 3 + controller.notifByteLen;
  size_t dataLen = dataLenWithoutCrc + 2;
  peri.buffs[Register::DataLen] = dataLen;
  peri.buffs[Register::ControllerType] =
      (uint8_t)ControllerAsI2c_asukiaaa::ControllerType::Xbox;
  const size_t dataCommandLen = 8;
  uint8_t dataCommandArr[dataCommandLen];
  String dataCommandStr = "";
  while (true) {
    btnTest.update();
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
#ifdef TEST_VIBRATION
        while (Serial.available()) {
          char v = Serial.read();
          Serial.print(v);
          if (v == '\n') {
            Serial.println();
            convertToDataArr(dataCommandStr, dataCommandArr, dataCommandLen);
            Serial.println("send " + dataCommandStr);
            for (int i = 0; i < dataCommandLen; ++i) {
              Serial.print(dataCommandArr[i]);
              Serial.print(" ");
            }
            controller.writeHIDReport(dataCommandArr, dataCommandLen);
            dataCommandStr = "";
          } else {
            if (v != '\r') {
              dataCommandStr += v;
            }
          }
        }
#endif
        if (btnTest.changedToPress()) {
          Serial.println("test");
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
