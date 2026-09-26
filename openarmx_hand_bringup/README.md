## openarmx_hand_bringup Overview

English | [中文](README_CN.md)

`openarmx_hand_bringup` provides two hardware modes for OpenArmX dual V10 arms with two LinkerHand O6 hands: a shared-CAN ros2_control system and independent O6 drivers on `can2/can3`.

The package uses `openarmx_hand_description` to compose the robot model without modifying the upstream `openarmx_description` or `openarmx_bringup` packages.

## 🧩 Main Files

| Path | Description |
|------|-------------|
| `launch/openarmx.bimanual.o6.launch.py` | Unified simulation and hardware launch entry |
| `config/controllers.yaml` | Arm, O6, and effort controller definitions |
| `config/o6.yaml` | Polling and publication settings for independent O6 drivers |
| `rviz/bimanual_o6.rviz` | RViz configuration for the combined robot |

## 🛠️ Build

From the ROS 2 workspace containing this repository:

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to openarmx_hand_bringup
source install/setup.bash
```

## 🚀 Launch

### Shared-Bus Hardware (Default)

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py
```

The default CAN assignment is:

| Device | Interface | CAN ID |
|--------|-----------|--------|
| Right arm | `can0` | Defined by the arm hardware |
| Left arm | `can1` | Defined by the arm hardware |
| Right O6 | Shares `can0` with the right arm | `39` (`0x27`) |
| Left O6 | Shares `can1` with the left arm | `40` (`0x28`) |

This is equivalent to passing `o6_hardware_mode:=shared_bus`. Only `can0` and `can1` are required. Both buses use classic CAN at 1 Mbps. Shared-bus mode does not support `can_fd=true`.

### Independent O6 Hardware

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=independent
```

The defaults are right arm `can0`, left arm `can1`, right O6 `can2/0x27`, and left O6 `can3/0x28`. This mode starts the standalone O6 drivers, joint-state mapper, and merger.

### Full Simulation

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  use_fake_hardware:=true
```

In `shared_bus` mode the arms and O6 hands switch between real and simulated hardware together. In `independent` mode, `use_fake_o6_hardware` can override the O6 mode separately:

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=independent \
  use_fake_hardware:=true use_fake_o6_hardware:=false
```

### Robot Namespace

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  robot_namespace:=robot1
```

The namespace is applied to nodes, controllers, and relative topics so multiple robot instances can be isolated.

## 🎛️ Arm Controllers

The default arm controller is `joint_trajectory_controller`. Select direct position forwarding with:

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  robot_controller:=forward_position_controller
```

Both controller modes command the seven joints of each arm. Separate left and right O6 position controllers command six actuated joints per hand. The controller manager runs at `100 Hz`.

## ⚖️ Gravity Compensation

Gravity compensation is disabled by default. Enable it with MIT arm control:

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  control_mode:=mit enable_forward_effort:=true
```

This starts the left and right forward-effort controllers after approximately one second and starts `openarmx_gravity_comp/gravity_comp_node` after approximately two seconds. The launch file enables both arms with `g_scale=1.05`. Enabling effort forwarding with `control_mode:=csp` is rejected.

## ⚙️ Launch Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `o6_hardware_mode` | `shared_bus` | Select `shared_bus` or `independent` |
| `use_fake_hardware` | `false` | Simulate the arms; also controls O6 in shared mode |
| `use_fake_o6_hardware` | empty | O6 simulation override in independent mode |
| `robot_namespace` | empty | Namespace for nodes, controllers, and topics |
| `left_can_interface` | `can1` | Left-arm CAN interface |
| `right_can_interface` | `can0` | Right-arm CAN interface |
| `left_o6_can_interface` | `can3` | Left O6 CAN interface in independent mode |
| `right_o6_can_interface` | `can2` | Right O6 CAN interface in independent mode |
| `left_o6_can_id` | `0x28` | Left O6 CAN ID |
| `right_o6_can_id` | `0x27` | Right O6 CAN ID |
| `o6_update_rate` | `50.0` | O6 asynchronous communication rate |
| `can_fd` | `false` | Must remain `false` in shared-bus mode |
| `control_mode` | `mit` | Arm hardware mode: `mit` or `csp` |
| `robot_controller` | `joint_trajectory_controller` | Arm position controller type |
| `enable_forward_effort` | `false` | Start effort controllers and gravity compensation |
| `launch_rviz` | `true` | Start RViz |

## 📡 State and Display Flow

In `shared_bus` mode, one `joint_state_broadcaster` publishes arm and O6 states on `joint_states`. In `independent` mode, the arm states and O6 mapper states are combined by `joint_state_merger` for display.

Both modes preserve `/openarmx/o6/left/command`, `/right/command`, and the corresponding `/state` topics, so existing HIGVR and VR command sources do not need to change.

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
