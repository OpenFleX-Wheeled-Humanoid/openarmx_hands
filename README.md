# Wheeled-Manipulator Dexterous Hand Usage Tutorial

**English** | [中文](README_CN.md)

## Terminal 1

### Start Shared-Bus Hardware (Default)

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  control_mode:=mit \
  robot_controller:=forward_position_controller \
  use_fake_hardware:=false \
  enable_forward_effort:=true
```

### Start Independent O6 Hardware

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=independent \
  right_o6_can_interface:=can6 \
  left_o6_can_interface:=can7 \
  control_mode:=mit \
  robot_controller:=forward_position_controller
```

### Simulation

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  use_fake_hardware:=true \
  use_rviz:=true
```

## Terminal 2

### Start VR Teleoperation

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_vr_teleop.launch.py vr_chassis:=true
```

## Terminal 4

### Start HIGVR Glove Input (with Joystick)

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_joystick.launch.py
```

## Inspect Data

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

## Terminal 5

### HIGVR-to-O6 Bridge

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

# Dual-Arm Dexterous Hand Usage Tutorial

## Terminal 1

### Start Shared-Bus Hardware (Default)

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  control_mode:=mit \
  robot_controller:=forward_position_controller \
  use_fake_hardware:=false \
  o6_hardware_mode:=shared_bus \
  enable_forward_effort:=true
```

### Start Independent O6 Hardware

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=independent \
  right_o6_can_interface:=can2 \
  left_o6_can_interface:=can3 \
  control_mode:=mit \
  robot_controller:=forward_position_controller
```

### Simulation

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=shared_bus \
  control_mode:=mit \
  robot_controller:=forward_position_controller \
  use_fake_hardware:=true
```

## Terminal 2

### Start the Pico Bridge

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 run openarmx_teleop_bridge_vr openarmx_teleop_bridge_vr_node
```

## Terminal 3

### Start IK Solving

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_teleop_vr teleop_vr.launch.py controller_pose_mode:="hand"
```

## Terminal 4

### Start HIGVR Glove Input

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
```

## 🧪 Inspect Data

```bash
ros2 topic echo /higvr/left_hand
ros2 topic echo /higvr/right_hand
ros2 topic echo /higvr/hand_frame
```

## Terminal 5

### HIGVR-to-O6 Bridge

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

See [`openarmx_hand_control/README.md`](openarmx_hand_control/README.md) for the complete HIGVR-to-O6 control flow and troubleshooting guide.

---

## Author

- **OpenArmX Team**
- Company: Chengdu Changshu Robotics Co., Ltd.
- Website: <https://openarmx.com/>

## Version

v0.1.0

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 Chengdu Changshu Robotics Co., Ltd.

For more details, see the [LICENSE](LICENSE) file or visit: <http://creativecommons.org/licenses/by-nc-sa/4.0/>

---

## Contact Us

### Chengdu Changshu Robotics Co., Ltd.

| Contact | Information |
|---------|-------------|
| Email | [openarmrobot@gmail.com](mailto:openarmrobot@gmail.com) |
| Phone / WeChat | +86-17746530375 |
| Website | <https://openarmx.com/> |
| Address | Huacheng Machinery Plant, No. 11 Xinye 8th Street, West Area, Tianjin Economic-Technological Development Area |
| Contact Person | Mr. Wang |
