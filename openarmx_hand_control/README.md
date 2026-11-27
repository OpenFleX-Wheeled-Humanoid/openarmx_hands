## openarmx_hand_control Overview

English | [中文](README_CN.md)

`openarmx_hand_control` contains the ROS 2 packages that connect HIGVR serial data gloves to the LinkerHand O6 hands used by OpenArmX.

This directory is a package collection, not an independent ROS 2 package. Build its two child packages with `colcon`.

## Included Packages

| Package | Responsibility | Documentation |
|---------|----------------|---------------|
| `openarmx_hands_hig` | Read and parse HIGVR serial glove data | [README](openarmx_hands_hig/README.md) |
| `openarmx_hands_bridge` | Map HIGVR values to O6 command messages | [README](openarmx_hands_bridge/README.md) |

## Control Flow

```text
HIGVR serial receiver
        │
        ▼
openarmx_hands_hig
        │
        ├── /higvr/left_hand  (std_msgs/msg/Int32MultiArray)
        └── /higvr/right_hand (std_msgs/msg/Int32MultiArray)
                    │
                    ▼
          openarmx_hands_bridge
                    │
                    ├── /openarmx/o6/left/command  (sensor_msgs/msg/JointState)
                    └── /openarmx/o6/right/command (sensor_msgs/msg/JointState)
                                      │
                                      ▼
                      O6 driver from openarmx_hand_bringup
                                      │
                                      ▼
                           Left and right O6 hands
```

`openarmx_hands_hig` only reads the glove. `openarmx_hands_bridge` only performs value mapping. The O6 hardware or simulation driver is provided separately by `openarmx_hand_bringup`.

## Build

From the workspace root:

```bash
cd ~/openflex_all/openflex_ws
colcon build --symlink-install --packages-select \
  openarmx_hands_hig \
  openarmx_hands_bridge
source install/setup.bash
```

To build the full O6 control path as well:

```bash
colcon build --symlink-install --packages-up-to \
  openarmx_hand_bringup \
  openarmx_hands_hig \
  openarmx_hands_bridge
source install/setup.bash
```

## Complete Dual-Hand Workflow

Open a sourced terminal for each process.

### 1. Start Arms and O6 Hands

Real hardware:

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  control_mode:=mit \
  robot_controller:=forward_position_controller \
  use_fake_hardware:=false \
  enable_forward_effort:=true
```

Simulation:

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  use_fake_hardware:=true \
  use_rviz:=true
```

The default real-hardware mapping is right O6 `can2/0x27` and left O6 `can3/0x28`.

### 2. Start HIGVR Input

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
```

### 3. Start the HIGVR-to-O6 Bridge

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

The bridge publishes continuously at `100 Hz` by default. Once valid glove data arrives, its mapped values are sent to the O6 command topics.

## Single-Hand Modes

Left hand:

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_left.launch.py
```

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=left
```

Right hand:

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_right.launch.py
```

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=right
```

## Inspect the Data Path

```bash
ros2 topic echo /higvr/left_hand
ros2 topic echo /higvr/right_hand
ros2 topic echo /openarmx/o6/left/command
ros2 topic echo /openarmx/o6/right/command
```

Check topic types and connections:

```bash
ros2 topic info /higvr/left_hand --verbose
ros2 topic info /openarmx/o6/left/command --verbose
```

## Default Data Mapping

HIGVR publishes six integer values in the order:

```text
[thumb flexion, index, middle, ring, little finger, thumb yaw]
```

The bridge converts the `0-100` HIGVR range to O6 command values and publishes:

```text
[thumb flexion, thumb yaw, index, middle, ring, little finger]
```

O6 values are clamped to `0-255`. Detailed mapping limits and reversal options are documented in [openarmx_hands_bridge/README.md](openarmx_hands_bridge/README.md).

## Serial Device Notes

The HIGVR driver uses `115200` baud by default and can discover `/dev/higvr_glove`, `/dev/serial/by-id/*`, `/dev/ttyACM*`, and `/dev/ttyUSB*` devices. For persistent serial access, add the current user to the `dialout` group and sign in again:

```bash
sudo usermod -aG dialout $USER
```

## Safety and Troubleshooting

- Verify the O6 command direction and range in simulation before controlling hardware.
- Keep the emergency stop accessible during initial hardware testing.
- If HIGVR topics are empty, check the serial device, permissions, baud rate, and `hand_mode`.
- If HIGVR topics update but O6 commands do not, check that `openarmx_hands_bridge` is running with the matching `hand` option.
- If O6 command topics update but the hands do not move, check the O6 driver, CAN interfaces, CAN IDs, and bringup mode.

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.

For details, see [LICENSE](LICENSE) or visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

## Author

- **OpenArmX Team**
- Company: Chengdu Changshu Robot Co., Ltd.
- Website: https://openarmx.com/

## Version

**Current Version**: 0.1.0
