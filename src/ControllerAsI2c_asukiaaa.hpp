#pragma once

#include <ControllerAsI2c_asukiaaa/XboxSeriesX.hpp>
#include <wire_asukiaaa.hpp>

#define CONTROLLER_AS_I2C_TARGET_ADDRESS 0x30

// #define CONTROLLER_AS_I2C_DEBUG_SERIAL Serial

namespace ControllerAsI2c_asukiaaa {

template <class Wire_t>
class Driver_template {
 public:
  Wire_t* wire;
  const uint8_t deviceAddress;

  Driver_template(Wire_t* wire,
                  uint8_t deviceAddress = CONTROLLER_AS_I2C_TARGET_ADDRESS)
      : deviceAddress(deviceAddress) {
    this->wire = wire;
    dataLen = Common::lengthDataHeader;
    dataArr = new uint8_t[dataLen];
    if (configWithoutCheckingPresence == NULL) {
      configWithoutCheckingPresence = new wire_asukiaaa::ReadConfig;
      configWithoutCheckingPresence->checkPresence = false;
      configWithoutCheckingPresence->stopBitAfterWritingAddress = true;
    }
  }

  ~Driver_template() {
    delete[] dataArr;
    if (xboxDataReadonly != NULL) {
      delete xboxDataReadonly;
    }
    if (xboxDataWritable != NULL) {
      delete xboxDataWritable;
    }
  }

  void beginWithoutWire() {}
  void begin() {
    wire->begin();
    beginWithoutWire();
  }

  uint8_t read() {
    readAt = millis();
    stateRead = wire_asukiaaa::checkPresence(wire, deviceAddress);
    if (stateRead != 0) {
      return stateRead;
    }
    stateRead = readHeader();
#ifdef CONTROLLER_AS_I2C_DEBUG_SERIAL
    CONTROLLER_AS_I2C_DEBUG_SERIAL.println("readHeader result " +
                                           String(stateRead));
#endif
    if (stateRead != 0) {
      return stateRead;
    }
    stateRead = readDataReadonly();
#ifdef CONTROLLER_AS_I2C_DEBUG_SERIAL
    CONTROLLER_AS_I2C_DEBUG_SERIAL.println("readDataReadonly result " +
                                           String(stateRead));
#endif
    return stateRead;
  }

  XboxSeriesX::DataWritable* getXboxSeriesXDataWritableP() {
    if (xboxDataWritable == NULL) {
      xboxDataWritable = new XboxSeriesX::DataWritable;
    }
    return xboxDataWritable;
  }

  uint8_t writeToXboxSeriesX() {
    auto writable = getXboxSeriesXDataWritableP();
    uint8_t data[XboxSeriesX::lengthWritable];
    size_t length = XboxSeriesX::lengthWritable;
    writable->communicationCount++;
    writable->toArr(data, length);
    int result;
    for (int i = 0; i < 2; ++i) {
      result = wire_asukiaaa::writeBytes(wire, deviceAddress,
                                         XboxSeriesX::Register::startWritable,
                                         data, length);
    }
    return result;
  }

  Common::Info getInfo() {
    Common::Info info;
    info.stateRead = stateRead;
    info.readAt = readAt;
    if (stateRead != 0) {
      return info;
    }
    if (header.controllerType == Common::ControllerType::XboxSeriesX &&
        xboxDataReadonly != NULL) {
      xboxDataReadonly->toInfo(&info);
      info.controllerType = Common::ControllerType::XboxSeriesX;
      info.connectionState = xboxDataReadonly->connectionState;
    }
    return info;
  }

 private:
  uint8_t* dataArr;
  size_t dataLen = 0;
  XboxSeriesX::DataReadonly* xboxDataReadonly = NULL;
  XboxSeriesX::DataWritable* xboxDataWritable = NULL;
  Common::DataHeader header;
  unsigned long readAt = 0;
  uint8_t stateRead = 1;
  static wire_asukiaaa::ReadConfig* configWithoutCheckingPresence;

  uint8_t readHeader(int retryCount = 2) {
    delayMicroseconds(10);
    auto result = wire_asukiaaa::readBytes(wire, deviceAddress, 0, dataArr,
                                           Common::lengthDataHeader,
                                           *configWithoutCheckingPresence);
    if (result == 0) {
      // #ifdef CONTROLLER_AS_I2C_DEBUG_SERIAL
      //       CONTROLLER_AS_I2C_DEBUG_SERIAL.println("data for header");
      //       for (int i = 0; i < Common::lengthDataHeader; ++i) {
      //         CONTROLLER_AS_I2C_DEBUG_SERIAL.print(String(dataArr[i]));
      //         CONTROLLER_AS_I2C_DEBUG_SERIAL.print(" ");
      //       }
      //       CONTROLLER_AS_I2C_DEBUG_SERIAL.println("");
      // #endif
      result = header.fromArr(dataArr, Common::lengthDataHeader);
    }
    if (result != 0) {
      if (retryCount > 0) {
#ifdef CONTROLLER_AS_I2C_DEBUG_SERIAL
        CONTROLLER_AS_I2C_DEBUG_SERIAL.println("retry");
#endif
        return readHeader(retryCount - 1);
      } else {
        return result;
      }
    }
    if (dataLen < header.getMaxLength()) {
      dataLen = header.getMaxLength();
      delete[] dataArr;
      dataArr = new uint8_t[dataLen];
    }
    prepareInstanceForControllerType();
    // #ifdef CONTROLLER_AS_I2C_DEBUG_SERIAL
    //     header.println(&CONTROLLER_AS_I2C_DEBUG_SERIAL);
    // #endif
    return result;
  }

  uint8_t readDataReadonly(int retryCount = 2) {
    auto startReadonly = XboxSeriesX::Register::startReadonly;
    auto lengthReadonly = header.lengthReadonly;
    if (lengthReadonly == 0) {
      return Common::Error::LengthUnmatch;
    }
    delayMicroseconds(10);
    auto result = wire_asukiaaa::readBytes(wire, deviceAddress, startReadonly,
                                           dataArr, lengthReadonly,
                                           *configWithoutCheckingPresence);
    if (result == 0) {
      // #ifdef CONTROLLER_AS_I2C_DEBUG_SERIAL
      //       CONTROLLER_AS_I2C_DEBUG_SERIAL.println("data for readonly");
      //       for (int i = 0; i < lengthReadonly; ++i) {
      //         CONTROLLER_AS_I2C_DEBUG_SERIAL.print(String(dataArr[i]));
      //         CONTROLLER_AS_I2C_DEBUG_SERIAL.print(" ");
      //       }
      //       CONTROLLER_AS_I2C_DEBUG_SERIAL.println("");
      // #endif
      result = xboxDataReadonly->fromArr(dataArr, lengthReadonly);
    }
    if (result != 0) {
      if (retryCount > 0) {
#ifdef CONTROLLER_AS_I2C_DEBUG_SERIAL
        CONTROLLER_AS_I2C_DEBUG_SERIAL.println("retry");
#endif
        return readDataReadonly(retryCount - 1);
      } else {
        return result;
      }
    }
    return result;
  }

  void prepareInstanceForControllerType() {
    if (header.controllerType == Common::ControllerType::XboxSeriesX) {
      if (xboxDataReadonly == NULL) {
        xboxDataReadonly = new XboxSeriesX::DataReadonly;
      }
    }
  }
};

template <class Wire_t>
wire_asukiaaa::ReadConfig*
    Driver_template<Wire_t>::configWithoutCheckingPresence = NULL;

typedef Driver_template<TwoWire> Driver;

}  // namespace ControllerAsI2c_asukiaaa
