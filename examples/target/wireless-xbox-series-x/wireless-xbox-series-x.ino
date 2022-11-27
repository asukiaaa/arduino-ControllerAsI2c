#include <Wire.h>
#include <button_asukiaaa.h>
#include <string_asukiaaa.h>

#include <ControllerAsI2c_asukiaaa.hpp>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <wire_asukiaaa.hpp>

namespace Xbox = ControllerAsI2c_asukiaaa::XboxSeriesX;
Xbox::DataWritable dataWrite;
Xbox::DataReadonly dataRead;
using ControllerAsI2c_asukiaaa::Common::ConnectionState;

#define PIN_BTN_TEST 0
// #define PIN_BTN_TEST 39 // m5stack btnA

// #define TEST_VIBRATION

button_asukiaaa::Button btnTest(PIN_BTN_TEST);

#ifdef TARGET_CONTROLLER_ADDRESS
XboxSeriesXControllerESP32_asukiaaa::Core controller(TARGET_CONTROLLER_ADDRESS);
#else
XboxSeriesXControllerESP32_asukiaaa::Core controller;
#endif

wire_asukiaaa::PeripheralHandler peri(&Wire);

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

void publishControllerNotif(XboxControllerNotificationParser& notif) {
  notif.toArr(dataRead.dataNotif, notif.expectedDataLen);
}

void taskController(void* param) {
  XboxControllerNotificationParser defaultControllerNotf;
  while (true) {
    controller.onLoop();
    if (controller.isConnected()) {
      if (controller.isWaitingForFirstNotification()) {
        dataRead.connectionState = ConnectionState::WaitingForFirstNotification;
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
        dataRead.connectionState = ConnectionState::Connected;
        publishControllerNotif(controller.xboxNotif);
      }
    } else {
      changedToConnected = false;
      dataRead.connectionState = controller.getCountFailedConnection() == 0
                                     ? ConnectionState::NotFound
                                     : ConnectionState::FoundButNotConnected;
      publishControllerNotif(defaultControllerNotf);
      if (controller.getCountFailedConnection() > 3) {
        Serial.println("failed connection");
        ESP.restart();
      }
    }
    dataRead.toArr(&peri.buffs[Xbox::Register::startReadonly],
                   Xbox::lengthReadonly);
    // Serial.println("put data");
    // for (int i = 0; i < Xbox::lengthReadonly; ++i) {
    //   Serial.print(String(peri.buffs[i + Xbox::Register::startReadonly]));
    //   Serial.print(" ");
    // }
    // Serial.println();
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
  Xbox::header.toArr(peri.buffs,
                     ControllerAsI2c_asukiaaa::Common::lengthDataHeader);
}

void loop() { delay(100); }
