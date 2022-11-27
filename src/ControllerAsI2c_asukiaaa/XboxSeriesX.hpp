#pragma once

#include <XboxControllerNotificationParser.h>

#include <XboxSeriesXHIDReportBuilder_asukiaaa.hpp>

#include "./Common.hpp"

namespace ControllerAsI2c_asukiaaa {
namespace XboxSeriesX {

static const uint8_t lengthReadonly =
    XboxControllerNotificationParser::expectedDataLen + 6 + 1 + 1 +
    2;  // 6: address, 1: connectionState, 1: battery, 2: crc
struct DataReadonly {
  uint8_t address[6];
  uint8_t dataNotif[XboxControllerNotificationParser::expectedDataLen];
  Common::ConnectionState connectionState;
  uint8_t battery;

  void toArr(uint8_t* data, size_t length) {
    if (length < lengthReadonly) {
      return;
    }
    memcpy(data, address, lenAddress);
    memcpy(&data[lenAddress], dataNotif, lenNotif);
    auto nextIndex = lenAddress + lenNotif;
    data[nextIndex++] = (uint8_t)connectionState;
    data[nextIndex++] = battery;
    Common::setCrc16OnTail(data, lengthReadonly);
  }

  int fromArr(uint8_t* dataArr, size_t dataLen) {
    if (dataLen != lengthReadonly) {
      return Common::Error::LengthUnmatch;
    }
    // Serial.println(String(crc16Created) + " " + String(crc16Received));
    if (!Common::detectMatchCrc16(dataArr, lengthReadonly)) {
      return Common::Error::CrcUnmatch;
    }
    memcpy(dataArr, dataArr, lenAddress);
    memcpy(dataNotif, &dataArr[lenAddress], lenNotif);
    auto nextIndex = lenAddress + lenNotif;
    connectionState = (Common::ConnectionState)dataArr[nextIndex++];
    battery = dataArr[nextIndex++];
    return 0;
  }

  void toInfo(Common::Info* info) {
    XboxControllerNotificationParser parser;
    parser.update(dataNotif, XboxControllerNotificationParser::expectedDataLen);
    info->buttonsDirL[0] = parser.btnDirUp;
    info->buttonsDirL[1] = parser.btnDirLeft;
    info->buttonsDirL[2] = parser.btnDirRight;
    info->buttonsDirL[3] = parser.btnDirDown;
    info->buttonsDirR[0] = parser.btnY;
    info->buttonsDirR[1] = parser.btnX;
    info->buttonsDirR[2] = parser.btnB;
    info->buttonsDirR[3] = parser.btnA;
    info->joyLHori = convertToJoyRate(parser.joyLHori, parser.maxJoy, true);
    info->joyLVert = convertToJoyRate(parser.joyLVert, parser.maxJoy, false);
    info->joyRHori = convertToJoyRate(parser.joyRHori, parser.maxJoy, true);
    info->joyRVert = convertToJoyRate(parser.joyRVert, parser.maxJoy, false);
    info->buttonsL[0] = parser.btnLB;
    info->buttonsL[1] = (float)parser.trigLT / parser.maxTrig;
    info->buttonsL[2] = parser.btnLS;
    info->buttonsR[0] = parser.btnRB;
    info->buttonsR[1] = (float)parser.trigRT / parser.maxTrig;
    info->buttonsR[2] = parser.btnRS;
    info->buttonsCenter[0] = parser.btnStart;
    info->buttonsCenter[1] = parser.btnSelect;
    info->buttonsCenter[2] = parser.btnXbox;
    info->buttonsCenter[3] = parser.btnShare;
  }

 private:
  static const size_t lenNotif =
      XboxControllerNotificationParser::expectedDataLen;
  static const size_t lenAddress = 6;

  static float convertToJoyRate(uint16_t val, uint16_t valMax,
                                bool zeroIsMinus) {
    uint16_t halfVal = valMax / 2;
    return ((float)val - halfVal) / halfVal * (zeroIsMinus ? 1 : -1);
  }
};

static const uint8_t lengthWritable =
    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase::arr8tLen + 2;
struct DataWritable {
  uint8_t
      dataReport[XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase::arr8tLen];
  void toArr(uint8_t* data, size_t length) {
    if (length < lengthWritable) {
      return;
    }
    size_t lenDataReport =
        XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase::arr8tLen;
    Common::setCrc16OnTail(data, lengthWritable);
  }
};

Common::DataHeader header = {.controllerType = Common::ControllerType::XboxSeriesX,
                             .lengthReadonly = lengthReadonly,
                             .lengthWritable = lengthWritable};

namespace Register {
uint8_t startStatic = 0;
uint8_t startReadonly = Common::lengthDataHeader;
uint8_t startWritable = startReadonly + lengthReadonly;
};  // namespace Register

};  // namespace XboxSeriesX
}  // namespace ControllerAsI2c_asukiaaa
