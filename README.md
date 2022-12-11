# arduino-ControllerAsI2c (ControllerAsI2c_asukiaaa)

A project to communicate controller via I2C.

## Usage

### Master controller

See [examples/master_controller](./examples/master_controller).

Example of log.
```
stateRead 0
readAt 310050
controllerType 1
receiverType 1
connectionState 0
communicationCount 84
buttonsDirL 0 0 0 0
buttonsDirR 0 0 0 0
joyLHori 0.00
joyLVert -0.00
joyRHori 0.00
joyRVert -0.00
buttonsL[0] 0.00
buttonsL[1] 0.00
buttonsL[2] 0.00
```

### Slave target

#### ESP32

See [examples/slave_target/esp32/](./examples/slave_target/esp32).

## License

MIT

## References

- [ESP32を受信機にしてXboxのコントローラーをI2C経由で使えるライブラリを作った](https://asukiaaa.blogspot.com/2022/12/esp32-as-receiver-of-xbox-series-x-controller.html)
- [XboxSeriesXControllerESP32_asukiaaa](https://github.com/asukiaaa/arduino-XboxSeriesXControllerESP32)
