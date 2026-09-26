## openarmx_hands_hig Overview
English | [中文](README_CN.md)

`openarmx_hands_hig` is a ROS 2 C++ driver for HIGVR serial data gloves. It supports left-hand, right-hand, and dual-hand input, with serial-port discovery, temporary permission handling, and automatic reconnection.

## 🧩 Main Files

| Path | Description |
|------|-------------|
| `src/higvr_hands_node.cpp` | Serial discovery, protocol parsing, hand detection, and topic publishing |
| `src/higvr_hands_joystick.cpp` | 12-column glove reader with calibration and VR topic publishing |
| `launch/higvr_hands.launch.py` | Generic launch file with a `hand_mode` selector |
| `launch/higvr_both.launch.py` | Dual-hand launch file |
| `launch/higvr_left.launch.py` | Left-hand launch file |
| `launch/higvr_right.launch.py` | Right-hand launch file |

## 🔌 Serial Protocol

- Default baud rate: `115200`
- Format: tab-separated ASCII text, one frame per line
- Frame fields: `[hand, finger1, finger2, finger3, finger4, finger5, back-of-hand opening]`
- Hand field: `1` for left and `2` for right
- Motion values: `0-100`; a smaller value usually means greater flexion

The node does not apply a `100 - value` inversion. To align the semantic finger order between hands, it reverses the first five values for the left hand by default and preserves the right-hand order. The sixth back-of-hand value remains last.

## 📡 Published Topics

| Topic | Message Type | Contents |
|-------|--------------|----------|
| `/higvr/left_hand` | `std_msgs/msg/Int32MultiArray` | Six left-hand motion values |
| `/higvr/right_hand` | `std_msgs/msg/Int32MultiArray` | Six right-hand motion values |
| `/higvr/left_accessories` | `std_msgs/msg/Int32MultiArray` | `[raw X, raw Y, joystick click, Y, X]` |
| `/higvr/right_accessories` | `std_msgs/msg/Int32MultiArray` | `[raw X, raw Y, joystick click, A, B]` |
| `/higvr/hand_frame` | `std_msgs/msg/Int32MultiArray` | `[hand, six hand values, five raw accessory values]` |
| `/higvr/raw` | `std_msgs/msg/String` | Raw serial line; disabled by default |

With `reverse_left_fingers:=true`, the left-hand order is `[finger5, finger4, finger3, finger2, finger1, back-of-hand opening]`. The default right-hand order is `[finger1, finger2, finger3, finger4, finger5, back-of-hand opening]`.

## 🛠️ Build

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install --packages-select openarmx_hands_hig
source install/setup.bash
```

## 🚀 Launch

Start with joystick support:

```bash
ros2 launch openarmx_hands_hig higvr_joystick.launch.py \
  hand_mode:=both port:=auto baud_rate:=115200
```

Start without joystick support:

```bash
# Both hands
ros2 launch openarmx_hands_hig higvr_both.launch.py

# Left hand
ros2 launch openarmx_hands_hig higvr_left.launch.py

# Right hand
ros2 launch openarmx_hands_hig higvr_right.launch.py

# Generic launch file
ros2 launch openarmx_hands_hig higvr_hands.launch.py \
  hand_mode:=both port:=auto baud_rate:=115200
```

The node reads 12-column glove frames. `/higvr/left_hand` and `/higvr/right_hand` retain the original six hand channels, while `/higvr/hand_frame` publishes the complete raw frame with accessories. The first two accessory values represent raw X and Y axes, item 3 is joystick click, and items 4 and 5 are left-hand Y/X or right-hand A/B. The node also converts joystick input with a fixed center and dead zone, publishes the `/pico_*` standard VR topics, and publishes buttons as `Bool`. Left and right hand IDs default to `1` and `2` and can be configured through parameters.

## ⚙️ Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `port` | `auto` | Serial device path or automatic discovery |
| `baud_rate` | `115200` | Serial baud rate |
| `hand_mode` | `both` | `left`, `right`, or `both` |
| `left_topic` | `/higvr/left_hand` | Left-hand topic |
| `right_topic` | `/higvr/right_hand` | Right-hand topic |
| `all_topic` | `/higvr/hand_frame` | Combined frame topic |
| `raw_topic` | `/higvr/raw` | Raw-text topic |
| `publish_raw` | `false` | Publish raw serial lines |
| `publish_vr_topics` | `true` | Publish `/pico_*` VR input topics |
| `joystick_center` | `510` | Joystick ADC center |
| `joystick_deadzone` | `100` | Raw ADC deadzone radius; center ±100 outputs zero |
| `joystick_minimum` | `0` | Joystick ADC minimum |
| `joystick_maximum` | `1023` | Joystick ADC maximum |
| `joystick_invert_x` | `true` | Invert X so left is negative and right positive |
| `joystick_invert_y` | `false` | Invert Y; default is up positive and down negative |
| `joystick_invert_left_y` | `true` | Invert left-hand Y to match chassis forward direction |
| `joystick_invert_right_y` | `false` | Invert right-hand Y axis |
| `reconnect_interval` | `1.0` | Reconnection interval in seconds |
| `auto_fix_permissions` | `true` | Try `sudo chmod a+rw` on permission failure |
| `reverse_left_fingers` | `true` | Reverse the first five left-hand values |
| `reverse_right_fingers` | `false` | Reverse the first five right-hand values |

The `/pico_*` compatibility topics follow the VR controller cadence: joystick axes are published continuously while active and emit one `0.0` sample on return to center; X/Y/A buttons and joystick-click publish `true` while held and one `false` on release, which keeps downstream timeout guards alive. Right-hand B toggles the head-control state on its press edge and publishes the new state; physical release does not change that state. A serial disconnect emits safe release/center samples. The raw `/higvr/*` topics remain frame-rate streams.

## 🔍 Discovery and Permissions

With `port:=auto`, the node searches in this order:

```text
/dev/higvr_glove
/dev/serial/by-id/*
/dev/ttyACM*
/dev/ttyUSB*
```

When two USB receivers are used, the node can open multiple ports and determine the hand from the first field in each frame. A device can also be selected explicitly:

```bash
ros2 launch openarmx_hands_hig higvr_both.launch.py port:=/dev/ttyACM0
```

`auto_fix_permissions` is enabled by default. If the serial device cannot be opened, the terminal may request a password to run:

```bash
sudo chmod a+rw <serial-device>
```

This permission is temporary. For persistent access, add the current user to `dialout` and sign in again:

```bash
sudo usermod -aG dialout $USER
```

## 🧪 Inspect Data

```bash
ros2 topic echo /higvr/left_hand
ros2 topic echo /higvr/right_hand
ros2 topic echo /higvr/hand_frame
ros2 topic echo /pico_left_controller/button_x
ros2 topic echo /pico_left_controller/button_y
ros2 topic echo /pico_left_controller/joystick_click
ros2 topic echo /pico_left_controller/joystick_x
ros2 topic echo /pico_left_controller/joystick_y
ros2 topic echo /pico_right_controller/button_a
ros2 topic echo /pico_right_controller/button_b
ros2 topic echo /pico_right_controller/joystick_click
ros2 topic echo /pico_right_controller/joystick_x
ros2 topic echo /pico_right_controller/joystick_y
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
