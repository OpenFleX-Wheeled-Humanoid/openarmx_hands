## hands_bringup Overview

English | [中文](README_CN.md)

`hands_bringup` provides the standalone ROS 2 launch entry for one or two LinkerHand O6 hands. It coordinates the O6 hardware driver, hand model, joint-state mapping, `robot_state_publisher`, and RViz.

This package starts the hands only. Use `openarmx_hand_bringup` when the O6 hands must be launched together with the dual-arm robot.

## 🧩 Main Files

| Path | Description |
|------|-------------|
| `launch/o6_bringup.launch.py` | Public launch entry |
| `bringup/o6_launch_common.py` | Launch arguments and node construction |

## 🛠️ Build

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to hands_bringup
source install/setup.bash
```

## 🚀 Simulation

```bash
# Both hands
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both

# One hand
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=left
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=right
```

Simulation does not start `o6_driver_node`. The joint-state mapper reads `/openarmx/o6/left/command` and `/openarmx/o6/right/command` directly.

## 🔌 CAN Hardware

```bash
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both left_can:=can0 right_can:=can1
```

The standalone defaults are left `can0` with ID `40` and right `can1` with ID `39`. Bring up the CAN interfaces at `1000000` bit/s before launching.

Start one hand with an explicit interface:

```bash
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=left left_can:=can0 left_can_id:=40
```

## 🔗 RS485 Hardware

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

The default Modbus baud rate is `115200`.

## ⚙️ Launch Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `sim` | `true` | Skip hardware drivers and use command topics |
| `hand` | `both` | `left`, `right`, or `both` |
| `left_can` | `can0` | Left-hand CAN interface |
| `right_can` | `can1` | Right-hand CAN interface |
| `left_can_id` | `40` | Left-hand CAN ID |
| `right_can_id` | `39` | Right-hand CAN ID |
| `transport` | `can` | `can` or `rs485` |
| `left_modbus_port` | `/dev/ttyUSB0` | Left-hand RS485 device |
| `right_modbus_port` | `/dev/ttyUSB1` | Right-hand RS485 device |
| `left_modbus_id` | `40` | Left-hand Modbus ID |
| `right_modbus_id` | `39` | Right-hand Modbus ID |
| `modbus_baudrate` | `115200` | RS485 baud rate |
| `driver_rate` | `100.0` | Hardware polling and state publish rate |
| `publish_rate` | `50.0` | URDF joint-state mapping rate |
| `info_rate` | `1.0` | Diagnostics and device information rate |
| `poll_touch` | `false` | Query standard pressure data |
| `poll_matrix_touch` | `false` | Query matrix pressure data |
| `poll_diagnostics` | `true` | Query temperature, current, and faults |
| `poll_device_info` | `true` | Query device metadata |
| `launch_rviz` | `true` | Start RViz |

## 📡 Node Flow

In hardware mode:

```text
/openarmx/o6/*/command -> hands_hardware/o6_driver_node -> O6 hardware
O6 hardware -> /openarmx/o6/*/state -> o6_joint_state_mapper_node -> /joint_states
```

`robot_state_publisher` consumes the mapped joint states and RViz displays the standalone hand model.

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
