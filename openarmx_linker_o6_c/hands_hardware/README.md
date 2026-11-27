## hands_hardware Overview

English | [中文](README_CN.md)

`hands_hardware` provides the C++ hardware layer for LinkerHand O6 devices. It contains CAN and RS485 drivers, a shared driver interface, and the `o6_driver_node` ROS 2 executable.

The node accepts six-value position and setting commands, communicates with one O6 hand, and publishes state, diagnostics, pressure data, and device information.

## 🧩 Components

| Component | Description |
|-----------|-------------|
| `o6_hardware_core` | Exported C++ CAN/RS485 driver library |
| `O6DriverBase` | Common driver interface |
| `O6CanDriver` | SocketCAN implementation |
| `O6Rs485Driver` | Serial Modbus-style implementation |
| `o6_driver_node` | ROS 2 wrapper for one left or right O6 hand |

## 🛠️ Build

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to hands_hardware
source install/setup.bash
```

## 🤖 Command Format

Position, speed, and torque arrays use six values in this order:

```text
[thumb flexion, thumb yaw, index, middle, ring, little]
```

Values are rounded and clamped to `0-255`. Missing position values are filled from the open pose; missing speed and torque values use the applicable command default.

At startup, the node sends speed `255`, torque `255`, and the open pose `[200, 255, 255, 255, 255, 255]` to all six channels.

## 🔌 Run with CAN

Bring up the CAN interface at `1000000` bit/s, then run one driver:

```bash
ros2 run hands_hardware o6_driver_node --ros-args \
  -p side:=right \
  -p transport:=can \
  -p can:=can2 \
  -p can_id:=39
```

Run a second node with `side:=left`, its own CAN interface, and ID `40` for the left hand.

## 🔗 Run with RS485

```bash
ros2 run hands_hardware o6_driver_node --ros-args \
  -p side:=right \
  -p transport:=rs485 \
  -p modbus_port:=/dev/ttyUSB0 \
  -p modbus_id:=39 \
  -p modbus_baudrate:=115200
```

The RS485 implementation supports position, speed, torque, current position, configured speed/torque, temperature, and fault registers. Some advanced sensing fields return placeholder values when the transport does not provide them.

## ⚙️ Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `side` | `right` | Topic side: `left` or `right` |
| `transport` | `can` | `can` or `rs485` |
| `can` | `can0` | SocketCAN interface |
| `can_id` | `39` | CAN device ID |
| `modbus_port` | `/dev/ttyUSB0` | RS485 serial device |
| `modbus_id` | `39` | RS485 device ID |
| `modbus_baudrate` | `115200` | Serial baud rate |
| `publish_rate` | `100.0` | Position polling and state publish rate |
| `info_rate` | `1.0` | Information and diagnostic rate |
| `init_speed` | six values of `255` | Startup speed values |
| `init_torque` | six values of `255` | Startup torque values |
| `poll_touch` | `false` | Query standard pressure data |
| `poll_matrix_touch` | `false` | Query matrix pressure data |
| `poll_diagnostics` | `true` | Query temperature, current, and faults |
| `poll_device_info` | `true` | Query device metadata |

## 📥 Subscribed Topics

The `side` parameter selects the topic prefix `/openarmx/o6/<side>`.

| Topic Suffix | Message Type | Purpose |
|--------------|--------------|---------|
| `/command` | `sensor_msgs/msg/JointState` | Six-value position command in `position` |
| `/setting_cmd` | `std_msgs/msg/String` | JSON speed or torque setting |

Setting examples:

```json
{"setting_cmd":"set_speed","params":{"speed":[255,255,255,255,255,255]}}
```

```json
{"setting_cmd":"set_torque","params":{"torque":[200,200,200,200,200,200]}}
```

## 📤 Published Topics

| Topic Suffix | Message Type | Purpose |
|--------------|--------------|---------|
| `/state` | `sensor_msgs/msg/JointState` | Current six-value hand state |
| `/info` | `std_msgs/msg/String` | Combined JSON device and diagnostic information |
| `/temperature` | `std_msgs/msg/Float32MultiArray` | Per-channel temperature |
| `/current` | `std_msgs/msg/Float32MultiArray` | Per-channel current |
| `/fault` | `std_msgs/msg/Int32MultiArray` | Per-channel fault values |
| `/device_info` | `std_msgs/msg/String` | Device metadata JSON |
| `/force` | `std_msgs/msg/Float32MultiArray` | Standard pressure data |
| `/matrix_touch` | `std_msgs/msg/String` | Matrix pressure JSON |
| `/matrix_touch_mass` | `std_msgs/msg/String` | Matrix pressure center/mass JSON |
| `/matrix_touch_pc` | `sensor_msgs/msg/PointCloud2` | Matrix pressure values as a point cloud |

Pressure topics are queried only when their corresponding polling parameters are enabled. If reading the current position throws an exception, the node republishes the most recently valid state and logs a warning.

## 💻 C++ API

The exported `hands_hardware::o6_hardware_core` target provides:

```cpp
#include "openarmx_linker_o6_c/drivers/can_driver.hpp"
#include "openarmx_linker_o6_c/drivers/rs485_driver.hpp"
```

Both implementations follow `O6DriverBase`, which defines position, speed, torque, state, diagnostics, pressure, and device-information operations.

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.

For details, visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

## Author

- **OpenArmX Team**
- Company: Chengdu Changshu Robot Co., Ltd.
- Website: https://openarmx.com/

## Version

**Current Version**: 0.0.0

## Acknowledgements

This package is part of the OpenArmX robotics ecosystem, developed for research and industrial applications in collaborative robotics.

---

## 📞 Contact Us

### Chengdu Changshu Robotics Co., Ltd.

| Contact | Details |
|---------|---------|
| 📧 Email | openarmrobot@gmail.com |
| 📱 Phone/WeChat | +86-17746530375 |
| 🌐 Website | <https://openarmx.com/> |
| 📍 Address | Huacheng Machinery Factory, No.11 Xinye 8th Street, West Area, TEDA |
| 👤 Contact Person | Mr. Wang |
