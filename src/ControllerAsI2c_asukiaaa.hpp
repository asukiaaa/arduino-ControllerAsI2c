#pragma once

#include <CRCx.h>
#include <XboxControllerNotificationParser.h>

#include <wire_asukiaaa.hpp>

#define CONTROLLER_AS_I2C_TARGET_ADDRESS 0x30

namespace ControllerAsI2c_asukiaaa {

namespace Register {
const uint8_t DataLen = 0;
const uint8_t ControllerType = 1;
const uint8_t ConnectionState = 2;
const uint8_t DataStart = 3;

namespace DataXbox {
const uint8_t NotifyArr = 0;
const uint8_t AddressArr = XboxControllerNotificationParser::expectedDataLen;
}  // namespace DataXbox

}  // namespace Register

namespace Error {
const uint8_t CrcUnmatch = 10;
}

enum class ControllerType : uint8_t { Unkown = 0, Xbox = 0 };
enum class ConnectionState : uint8_t {
  Unkown = 9,
  Connected = 0,
  WaitingForFirstNotification = 1,
  FoundButNotConnected = 2,
  NotFound = 3,
};

class Info {
 public:
  int stateRead = -1;
  unsigned long readAt = 0;
  ControllerType controllerType = ControllerType::Unkown;
  ConnectionState connectionState = ConnectionState::Unkown;
  int8_t buttonsDirL[4];
  int8_t buttonsDirR[4];
  float joyLHori;
  float joyLVert;
  float joyRHori;
  float joyRVert;
  float buttonsL[3];
  float buttonsR[3];
  // 0: start, 1: select
  uint8_t buttonsCenter[4];

  static const int dataMaxLen = 50;
  uint8_t dataArr[dataMaxLen];
  uint8_t dataLen = 0;

  void print(Stream* serial) {
    serial->println("stateRead " + String(stateRead));
    serial->println("readAt " + String(readAt));
    // for (int i = 0; i < dataLen; ++i) {
    //   serial->print(" ");
    //   serial->print(dataArr[i]);
    //   if (i % 10 == 9) {
    //     serial->println();
    //   }
    // }
    serial->println();
    if (stateRead != 0) {
      return;
    }
    serial->println("controllerType " + String((uint8_t)controllerType));
    serial->println("connectionState " + String((uint8_t)connectionState));
    if (connectionState == ConnectionState::Connected) {
      serial->print("buttonsDirL");
      for (int i = 0; i < 4; ++i) {
        serial->print(" ");
        serial->print(buttonsDirL[i]);
      }
      serial->println();
      serial->print("buttonsDirR");
      for (int i = 0; i < 4; ++i) {
        serial->print(" ");
        serial->print(buttonsDirR[i]);
      }
      serial->println();
      serial->println("joyLHori " + String(joyLHori));
      serial->println("joyLVert " + String(joyLVert));
      serial->println("joyRHori " + String(joyRHori));
      serial->println("joyRVert " + String(joyRVert));
      for (int i = 0; i < 3; ++i) {
        serial->print("buttonsL[" + String(i) + "] ");
        serial->println(buttonsL[i]);
      }
      for (int i = 0; i < 3; ++i) {
        serial->print("buttonsR[" + String(i) + "] ");
        serial->println(buttonsR[i]);
      }
      serial->print("buttonsCenter");
      for (int i = 0; i < 4; ++i) {
        serial->print(" ");
        serial->print(buttonsCenter[i]);
      }
      serial->println();
    }
  }

  int parse() {
    dataLen = dataArr[0];
    uint16_t crc16Created = crcx::crc16(dataArr, dataLen - 2);
    uint16_t crc16Received =
        ((uint16_t)dataArr[dataLen - 2] << 8) + dataArr[dataLen - 1];
    // Serial.println(String(crc16Created) + " " + String(crc16Received));
    if (crc16Created != crc16Received) {
      return stateRead = Error::CrcUnmatch;
    }
    controllerType = (ControllerType)dataArr[Register::ControllerType];
    connectionState = (ConnectionState)dataArr[Register::ConnectionState];
    if (connectionState == ConnectionState::Connected) {
      if (controllerType == ControllerType::Xbox) {
        parseForXbox();
      }
    }
    return 0;
  }

 private:
  static float convertToJoyRate(uint16_t val, uint16_t valMax,
                                bool zeroIsMinus) {
    uint16_t halfVal = valMax / 2;
    return ((float)val - halfVal) / halfVal * (zeroIsMinus ? 1 : -1);
  }

  void parseForXbox() {
    XboxControllerNotificationParser parser;
    parser.update(&dataArr[Register::DataStart],
                  XboxControllerNotificationParser::expectedDataLen);
    buttonsDirL[0] = parser.btnDirUp;
    buttonsDirL[1] = parser.btnDirLeft;
    buttonsDirL[2] = parser.btnDirRight;
    buttonsDirL[3] = parser.btnDirDown;
    buttonsDirR[0] = parser.btnY;
    buttonsDirR[1] = parser.btnX;
    buttonsDirR[2] = parser.btnB;
    buttonsDirR[3] = parser.btnA;
    joyLHori = convertToJoyRate(parser.joyLHori, parser.maxJoy, true);
    joyLVert = convertToJoyRate(parser.joyLVert, parser.maxJoy, false);
    joyRHori = convertToJoyRate(parser.joyRHori, parser.maxJoy, true);
    joyRVert = convertToJoyRate(parser.joyRVert, parser.maxJoy, false);
    buttonsL[0] = parser.btnLB;
    buttonsL[1] = (float)parser.trigLT / parser.maxTrig;
    buttonsL[2] = parser.btnLS;
    buttonsR[0] = parser.btnRB;
    buttonsR[1] = (float)parser.trigRT / parser.maxTrig;
    buttonsR[2] = parser.btnRS;
    buttonsCenter[0] = parser.btnStart;
    buttonsCenter[1] = parser.btnSelect;
    buttonsCenter[2] = parser.btnXbox;
    buttonsCenter[3] = parser.btnShare;
  }
};

template <class Wire_t>
class Driver_template {
 public:
  Wire_t* wire;
  Driver_template(Wire_t* wire) { this->wire = wire; }

  void beginWithoutWire() {}

  void begin() {
    wire->begin();
    beginWithoutWire();
  }

  Info read() {
    Info info;
    auto result =
        wire_asukiaaa::readBytes(wire, CONTROLLER_AS_I2C_TARGET_ADDRESS, 0,
                                 info.dataArr, info.dataMaxLen);
    info.stateRead = result;
    info.readAt = millis();
    if (result != 0) {
      return info;
    }
    info.parse();
    return info;
  }

 private:
};

typedef Driver_template<TwoWire> Driver;

}  // namespace ControllerAsI2c_asukiaaa
