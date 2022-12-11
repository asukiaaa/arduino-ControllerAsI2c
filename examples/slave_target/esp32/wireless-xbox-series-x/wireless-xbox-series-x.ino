#include <Wire.h>

#include <ControllerAsI2c_asukiaaa.hpp>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <wire_asukiaaa.hpp>

namespace Xbox = ControllerAsI2c_asukiaaa::XboxSeriesX;
Xbox::DataWritable dataWrite;
Xbox::DataReadonly dataRead;
using ControllerAsI2c_asukiaaa::Common::ConnectionState;

#ifdef TARGET_CONTROLLER_ADDRESS
XboxSeriesXControllerESP32_asukiaaa::Core controller(TARGET_CONTROLLER_ADDRESS);
#else
XboxSeriesXControllerESP32_asukiaaa::Core controller;
#endif

wire_asukiaaa::PeripheralHandler peri(&Wire);

bool changedToConnected = false;

void publishControllerNotif(XboxControllerNotificationParser& notif) {
  notif.toArr(dataRead.dataNotif, notif.expectedDataLen);
}

bool neededToHandleWritable = false;
uint8_t handledCommunicationCount = 0;

void onReceiveAddress(uint8_t address, uint8_t data) {
  if (address == Xbox::Register::startWritable + Xbox::lengthWritable - 1) {
    neededToHandleWritable = true;
  }
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
        dataRead.connectionState = ConnectionState::Connected;
        publishControllerNotif(controller.xboxNotif);
      }
      if (neededToHandleWritable) {
        Serial.println("handle writable info.");
        neededToHandleWritable = false;
        Xbox::DataWritable writable;
        auto err = writable.fromArr(&peri.buffs[Xbox::Register::startWritable],
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
      neededToHandleWritable = false;
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
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  controller.begin();
  peri.setOnReceiveForAddress(onReceiveAddress);
  Wire.onReceive([](int i) { peri.onReceive(i); });
  Wire.onRequest([]() {
    dataRead.communicationCount++;
    peri.onRequest();
  });
  Wire.begin((uint8_t)CONTROLLER_AS_I2C_TARGET_ADDRESS, SDA, SCL, 0);
  // loop handles oncore0, last vlue of xTaskCreate means core number
  xTaskCreatePinnedToCore(taskController, "taskController", 4096, NULL, 1, NULL,
                          1);
  Xbox::header.toArr(peri.buffs,
                     ControllerAsI2c_asukiaaa::Common::lengthDataHeader);
}

void loop() { delay(100); }
