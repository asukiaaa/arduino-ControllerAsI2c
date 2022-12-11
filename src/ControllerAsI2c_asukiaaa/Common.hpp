#pragma once

#include <CRCx.h>

namespace ControllerAsI2c_asukiaaa {

namespace Common {
enum class ControllerType : uint8_t { Unkown = 0, XboxSeriesX = 1 };
enum class ConnectionState : uint8_t {
  Unkown = 9,
  Connected = 0,
  WaitingForFirstNotification = 1,
  FoundButNotConnected = 2,
  NotFound = 3,
};
namespace Error {
const uint8_t CrcUnmatch = 10;
const uint8_t LengthUnmatch = 11;
const uint8_t UnkownControllerType = 12;
}  // namespace Error

void setCrc16OnTail(uint8_t* dataArr, uint8_t dataLenIncludesCrc) {
  uint16_t crc16 = crcx::crc16(dataArr, dataLenIncludesCrc - 2);
  dataArr[dataLenIncludesCrc - 2] = crc16 >> 8;
  dataArr[dataLenIncludesCrc - 1] = crc16 & 0xff;
}

bool detectMatchCrc16(uint8_t* dataArr, size_t dataLenIncludesCrc) {
  uint16_t crc16Created = crcx::crc16(dataArr, dataLenIncludesCrc - 2);
  uint16_t crc16Received = ((uint16_t)dataArr[dataLenIncludesCrc - 2] << 8) +
                           dataArr[dataLenIncludesCrc - 1];
  return crc16Created == crc16Received;
}

static const uint8_t lengthDataHeader = 6;
struct DataHeader {
  ControllerType controllerType;
  uint8_t lengthReadonly;
  uint8_t lengthWritable;

  void toArr(uint8_t* data, size_t length) {
    if (length < lengthDataHeader) {
      return;
    }
    data[0] = (uint8_t)controllerType;
    data[1] = lengthReadonly;
    data[2] = lengthWritable;
    setCrc16OnTail(data, lengthDataHeader);
  }

  uint8_t fromArr(uint8_t* data, size_t dataLen) {
    if (dataLen != lengthDataHeader) {
      return Error::LengthUnmatch;
    }
    auto type = (ControllerType)data[0];
    if (type == ControllerType::Unkown) {
      return Error::UnkownControllerType;
    }
    controllerType = type;
    lengthReadonly = data[1];
    lengthWritable = data[2];
    return 0;
  }

  size_t getMaxLength() {
    return max(lengthDataHeader, max(lengthReadonly, lengthWritable));
  }

  void println(Stream* serial) {
    serial->println("controllerType: " + String((uint8_t)controllerType));
    serial->println("lengthReadonly: " + String(lengthReadonly));
    serial->println("lengthWritable: " + String(lengthWritable));
  }
};

class Info {
 public:
  int stateRead = -1;
  unsigned long readAt = 0;
  Common::ControllerType controllerType = Common::ControllerType::Unkown;
  Common::ConnectionState connectionState = Common::ConnectionState::Unkown;
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
    if (stateRead != 0) {
      return;
    }
    serial->println("controllerType " + String((uint8_t)controllerType));
    serial->println("connectionState " + String((uint8_t)connectionState));
    if (connectionState == Common::ConnectionState::Connected) {
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
};

}  // namespace Common
}  // namespace ControllerAsI2c_asukiaaa
