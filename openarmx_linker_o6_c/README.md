## openarmx_linker_o6_c Overview

English | [中文](README_CN.md)

`openarmx_linker_o6_c` is the ROS 2 stack for LinkerHand O6. It contains three independent packages:

| Package | Description |
|---------|-------------|
| `hands_hardware` | CAN/RS485 drivers and the O6 hardware node |
| `hands_description` | Left/right URDFs, RViz configuration, and joint-state mapping |
| `hands_bringup` | Unified simulation and hardware launch files |

O6 commands use the order `[thumb flexion, thumb yaw, index, middle, ring, little]` and the range `0-255`. A value near `255` is open and a value near `0` is closed. The default open pose is `[200, 255, 255, 255, 255, 255]`.

## 🛠️ Build

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install \
  --packages-select hands_description hands_hardware hands_bringup
source install/setup.bash
```

## 🚀 Launch

### Simulation and RViz

```bash
# Both hands
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both

# One hand
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=left
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=right
```

### CAN Hardware

```bash
# Both hands
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both left_can:=can0 right_can:=can1

# Left hand
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=left left_can:=can0

# Right hand
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=right right_can:=can1
```

The default CAN IDs are `40` for the left hand and `39` for the right hand. Override them with `left_can_id` and `right_can_id`.

Bring up an inactive CAN interface before launching the driver:

```bash
sudo ip link set can0 down
sudo ip link set can0 up type can bitrate 1000000
```

### RS485 Hardware

```bash
# Right hand
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=right transport:=rs485 \
  right_modbus_port:=/dev/ttyUSB0 right_modbus_id:=39

# Both hands
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both transport:=rs485 \
  left_modbus_port:=/dev/ttyUSB0 right_modbus_port:=/dev/ttyUSB1
```

The default Modbus baud rate is `115200`; override it with `modbus_baudrate`.

## ⚙️ Main Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `sim` | `true` | Use models when `true`; start hardware drivers when `false` |
| `hand` | `both` | `left`, `right`, or `both` |
| `transport` | `can` | `can` or `rs485` |
| `launch_rviz` | `true` | Start RViz |
| `driver_rate` | `100.0` | Hardware driver loop rate |
| `publish_rate` | `50.0` | Joint-state mapper publish rate |
| `info_rate` | `1.0` | Information and diagnostic topic rate |
| `poll_diagnostics` | `true` | Query temperature, current, and fault state |
| `poll_device_info` | `true` | Query device information |
| `poll_touch` | `false` | Publish standard `/force` data |
| `poll_matrix_touch` | `false` | Publish matrix-touch topics |

## 🧤 HIGVR Glove Control

Start the glove driver, O6, and the bridge in order:

```bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

For hardware, change the second command to `sim:=false` and supply the CAN parameters.

## 🖥️ Manual Control GUI

Start O6 in simulation or hardware mode, then run:

```bash
ros2 run openarmx_hand_gui o6_manual_control_gui
```

The GUI publishes to `/openarmx/o6/left/command`, `/openarmx/o6/right/command`, and the corresponding `setting_cmd` topics.

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.

For details, see [LICENSE](LICENSE) or visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

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
