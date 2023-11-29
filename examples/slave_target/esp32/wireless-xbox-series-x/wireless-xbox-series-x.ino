#include <Wire.h>
#include <esp_task_wdt.h>
#define WDT_TIMEOUT 10

#include <ControllerAsI2c_asukiaaa.hpp>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <wire_asukiaaa.hpp>

// specify controller address
// #define XBOX_CONTROLLER_ADDRESS "aa:bb:cc:dd:ee:ff"

namespace Xbox = ControllerAsI2c_asukiaaa::XboxSeriesX;
Xbox::DataWritable dataWrite;
Xbox::DataReadonly dataRead;
using ControllerAsI2c_asukiaaa::Common::ConnectionState;

#ifdef XBOX_CONTROLLER_ADDRESS
XboxSeriesXControllerESP32_asukiaaa::Core controller(XBOX_CONTROLLER_ADDRESS);
#else
XboxSeriesXControllerESP32_asukiaaa::Core controller;
#endif

class I2cManager
    : public wire_asukiaaa::
          PeripheralHandlerSeparateReceiveAndSendBytesTemplate<TwoWire> {
 public:
  I2cManager(TwoWire* wire)
      : wire_asukiaaa::PeripheralHandlerSeparateReceiveAndSendBytesTemplate<
            TwoWire>(wire) {}

  void onReceiveForAddress(uint8_t address, uint8_t data) {
    if (address == Xbox::Register::startWritable + Xbox::lengthWritable - 1) {
      neededToHandleWritable = true;
    }
  }

  void onSendFromAddress(uint8_t address) {
    if (address == Xbox::Register::startReadonly) {
      ++countSendReadonlyInfo;
    } else if (address == Xbox::Register::startStatic) {
      ++countSendStaticInfo;
    }
  }

  bool neededToHandleWritable = false;
  uint8_t countSendStaticInfo = 0;
  uint8_t countSendReadonlyInfo = 0;
} peri(&Wire);

#define MS_TO_RESET_FOR_NOT_CONNECTED_FROM_FOUND 10000UL

#ifndef PIN_SDA
#define PIN_SDA SDA
#endif
#ifndef PIN_SCL
#define PIN_SCL SCL
#endif

bool changedToConnected = false;

struct FoundInfo {
  bool found = false;
  unsigned long foundAt = 0;

  void reset() { found = false; }

  bool neededToResetWhenNotConnected() {
    return found &&
           millis() - foundAt > MS_TO_RESET_FOR_NOT_CONNECTED_FROM_FOUND;
  }
};
FoundInfo foundInfo;

void publishControllerNotif(XboxControllerNotificationParser& notif) {
  notif.toArr(dataRead.dataNotif, notif.expectedDataLen);
}

uint8_t handledCommunicationCount = 0;

void taskController(void* param) {
  XboxControllerNotificationParser defaultControllerNotf;
  while (true) {
    controller.onLoop();
    if (controller.isConnected()) {
      foundInfo.reset();
      if (controller.isWaitingForFirstNotification()) {
        dataRead.connectionState = ConnectionState::WaitingForFirstNotification;
      } else {
        if (!changedToConnected) {
          changedToConnected = true;
          Serial.println("Connected to " + controller.buildDeviceAddressStr());
        }
        dataRead.connectionState = ConnectionState::Connected;
        dataRead.battery = controller.battery;
        publishControllerNotif(controller.xboxNotif);
      }
      if (peri.neededToHandleWritable) {
        Serial.println("handle writable info.");
        peri.neededToHandleWritable = false;
        Xbox::DataWritable writable;
        auto err =
            writable.fromArr(&peri.bytesReceive[Xbox::Register::startWritable],
                             Xbox::lengthWritable);
        if (err == 0 &&
            writable.communicationCount != handledCommunicationCount) {
          Serial.println("handle. communicationCount:" +
                         String(writable.communicationCount));
          handledCommunicationCount = writable.communicationCount;
          controller.writeHIDReport(writable.report);
        } else {
          Serial.println(
              "not handled. error:" + String(err) +
              " communicationCount:" + String(writable.communicationCount));
        }
      }
    } else {
      changedToConnected = false;
      peri.neededToHandleWritable = false;
      dataRead.connectionState = controller.getCountFailedConnection() == 0
                                     ? ConnectionState::NotFound
                                     : ConnectionState::FoundButNotConnected;
      if (!foundInfo.found && controller.getCountFailedConnection() > 0) {
        foundInfo.found = true;
        foundInfo.foundAt = millis();
      }
      publishControllerNotif(defaultControllerNotf);
      if (controller.getCountFailedConnection() > 3 ||
          foundInfo.neededToResetWhenNotConnected()) {
        Serial.println("failed connection");
#ifdef ESP32
        ESP.restart();
#endif
      }
    }
    dataRead.toArr(&peri.bytesSend[Xbox::Register::startReadonly],
                   Xbox::lengthReadonly);
    delay(10);
  }
}

void setup() {
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.begin(115200);
  controller.begin();
  Wire.onReceive([](int i) {
    peri.onReceive(i);
    // Serial.println("onReceive " + String(millis()) + " " +
    //                String(peri.getBuffIndex()));
  });
  Wire.onRequest([]() {
    dataRead.communicationCount++;
    peri.onRequest();
  });
#ifdef ESP32
  Wire.begin((uint8_t)CONTROLLER_AS_I2C_TARGET_ADDRESS, PIN_SDA, PIN_SCL, 0);
#else
  Wire.begin(CONTROLLER_AS_I2C_TARGET_ADDRESS);
#endif
  // loop handles on core0, last value of xTaskCreate means core number
#ifdef ESP32
  xTaskCreatePinnedToCore(taskController, "taskController", 4096, NULL, 1, NULL,
                          1);
#endif
  auto header = Xbox::header;
  header.receiverType = ControllerAsI2c_asukiaaa::Common::ReceiverType::esp32;
  header.toArr(peri.bytesSend,
               ControllerAsI2c_asukiaaa::Common::lengthDataHeader);
}

void loop() {
  // Serial.println("hello");
  // taskController();
#ifdef ESP32
  static uint8_t resetedCountSendStaticInfo = 0;
  static uint8_t resetedCountSendReadonlyInfo = 0;
  if (resetedCountSendReadonlyInfo != peri.countSendReadonlyInfo &&
      resetedCountSendStaticInfo != peri.countSendStaticInfo) {
    resetedCountSendReadonlyInfo = peri.countSendReadonlyInfo;
    resetedCountSendStaticInfo = peri.countSendStaticInfo;
    esp_task_wdt_reset();
  }
  delay(1000);
  // esp_task_wdt_reset();
#else
  taskController(0);
#endif
}
