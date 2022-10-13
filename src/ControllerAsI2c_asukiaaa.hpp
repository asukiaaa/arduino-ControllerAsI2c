#pragma once

#include <CRCx.h>
#include <XboxControllerNotificationParser.h>

#include <wire_asukiaaa.hpp>

#define CONTROLLER_AS_I2C_TARGET_ADDRESS 0x20

namespace ControllerAsI2c_asukiaaa {

namespace Register {
const uint8_t dataLen = 0;
const uint8_t controllerType = 1;
const uint8_t controllerState = 2;
const uint8_t dataStart = 3;
}  // namespace Register

namespace Error {
const uint8_t crcUnmatch = 10;
}

enum class ControllerType : uint8_t { Unkown = 0, Xbox = 0 };
enum class ConectionState : uint8_t {
  connected = 0,
  foundButNotConnected = 1,
  notFound = 2,
};

class Info {
 public:
  int stateRead = -1;
  ControllerType controllerType;
  int8_t buttonsLeftInt[4];
  int8_t buttonsRightInt[4];
  int16_t joyLeftHori;
  int16_t joyLeftVert;
  int16_t joyRightHori;
  int16_t joyRightVert;

  static const int dataMaxLen = 50;
  uint8_t dataArr[dataMaxLen];
  uint8_t dataLen = 0;

  Info() {}

  void print(Stream* serial) {
    serial->println("state read " + String(stateRead));
  }

 private:
};

template <class Wire_t>
class Driver {
 public:
  Wire_t* wire;
  Driver(Wire_t* wire) { this->wire = wire; }

  void beginWithoutWire() {}

  void begin() {
    wire->begin();
    beginWithoutWire();
  }

  int read(Info* info) {
    auto result =
        wire_asukiaaa::readBytes(wire, CONTROLLER_AS_I2C_TARGET_ADDRESS, 0,
                                 info->dataArr, info->dataMaxLen);
    info->stateRead = result;
    if (result != 0) {
      return result;
    }
    uint8_t dataLen = info->dataArr[0];
    uint16_t crc16Created = crcx::crc16(info->dataArr, dataLen);
    uint16_t crc16Received =
        ((uint16_t)info->dataArr[dataLen] << 8) + info->dataArr[dataLen + 1];
    if (crc16Created != crc16Received) {
      return info->stateRead = Error::crcUnmatch;
    }
    if (info->dataArr[Register::controllerState] ==
        (uint8_t)ConectionState::connected) {
      XboxControllerNotificationParser parser;
      parser.update(&info->dataArr[Register::dataStart],
                    XboxControllerNotificationParser::expectedDataLen);
      Serial.println(parser.toString());
    }

    for (int i = 0; i < info->dataMaxLen; ++i) {
      Serial.print(" ");
      Serial.print(info->dataArr[i]);
      if (i % 10 == 9) {
        Serial.println();
      }
    }
    Serial.println();
    return 0;
  }

 private:
};

}  // namespace ControllerAsI2c_asukiaaa
