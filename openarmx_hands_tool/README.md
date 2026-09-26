## openarmx_hands_tool Overview

English | [中文](README_CN.md)

`openarmx_hands_tool` provides utilities for OpenArmX hand devices, including an O6 manual control GUI, CAN interface scripts, and an O6 motor-status query tool.

## 🧩 Included Tools

| Tool | Description |
|------|-------------|
| `openarmx_hand_gui` | ROS 2 manual control GUI for O6 hands |
| `openarmx_hands_interface/can_up.py` | Bring up physical CAN interfaces |
| `openarmx_hands_interface/can_down.py` | Bring down physical CAN interfaces |
| `openarmx_hands_interface/check_hand_status.py` | Query O6 position, speed, torque, temperature, fault, and current |
| `openarmx_hands_interface/check_motor_status.py` | Check both arms and O6 hands in independent-CAN mode |
| `openarmx_hands_interface/check_motor_status_shared_bus.py` | Check both arms and O6 hands in the default shared-bus mode |

## 🛠️ Build the GUI

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install --packages-select openarmx_hand_gui
source install/setup.bash
```

Run the GUI:

```bash
ros2 run openarmx_hand_gui o6_manual_control_gui
```

See the [GUI usage guide](openarmx_hand_gui/README.md) for usage details.

See [`openarmx_hands_interface/README.md`](openarmx_hands_interface/README.md) for CAN setup and hardware-status diagnostics.

## 🔌 Bring Up CAN

Enter the tools directory:

```bash
cd <openarmx_ws_hand>
cd src/openarmx_hands/src/openarmx_hands_tool
```

Bring up all detected physical CAN interfaces at the default bitrate of `1000000`:

```bash
python3 openarmx_hands_interface/can_up.py
```

Select an interface or bitrate:

```bash
python3 openarmx_hands_interface/can_up.py can0
python3 openarmx_hands_interface/can_up.py can0 --bitrate 1000000
```

The script runs `sudo ip link`, so the terminal may request a password.

## ⏹️ Bring Down CAN

```bash
# Bring down all active physical CAN interfaces
python3 openarmx_hands_interface/can_down.py

# Bring down one interface
python3 openarmx_hands_interface/can_down.py can0
```

## 🧪 Query O6 Status

Query all active physical CAN interfaces:

```bash
python3 openarmx_hands_interface/check_hand_status.py
```

Query selected interfaces and show probes that do not respond:

```bash
python3 openarmx_hands_interface/check_hand_status.py can2 can3 --show-missing
```

The default IDs are `0x27` (decimal `39`) for the right hand and `0x28` (decimal `40`) for the left hand. To override the IDs and timeout:

```bash
python3 openarmx_hands_interface/check_hand_status.py can2 \
  --right-id 0x27 --left-id 0x28 --timeout 0.3
```

## Check the Complete Robot

For the default shared-bus mode, where each arm and hand share one CAN interface:

```bash
python3 openarmx_hands_interface/check_motor_status_shared_bus.py
```

For independent-CAN mode:

```bash
python3 openarmx_hands_interface/check_motor_status.py
```

The default mapping is right arm `can0`, left arm `can1`, right O6 `can2/0x27`, and left O6 `can3/0x28`. The arm check explicitly queries motor IDs `1-7` and never queries the removed gripper motor ID `8`.

Override the CAN interfaces when required:

```bash
python3 openarmx_hands_interface/check_motor_status.py \
  --right-arm-can can0 --left-arm-can can1 \
  --right-hand-can can2 --left-hand-can can3
```

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
