## hands_description Overview

English | [中文](README_CN.md)

`hands_description` provides LinkerHand O6 URDF assets, RViz configuration, range-to-joint mapping, and robot-description generation for ROS 2. It supports left, right, and dual-hand standalone models.

This package contains no CAN or RS485 communication. Hardware access is implemented by `hands_hardware`.

## 🧩 Components

| Component | Description |
|-----------|-------------|
| `o6_description_core` | C++ library for O6 pose normalization, URDF mapping, and model generation |
| `o6_joint_state_mapper_node` | Converts six-value O6 states or commands into URDF joint states |
| `o6_robot_description_node` | Builds a model and stores it in the node's `robot_description` parameter |
| `o6_robot_description_cli` | Prints a generated O6 URDF to standard output |
| `urdf/o6/` | Left and right O6 models and mesh assets |
| `rviz/o6.rviz` | Standalone O6 RViz configuration |

## 🛠️ Build

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-select hands_description
source install/setup.bash
```

## 🤖 O6 Pose Format

The six input values use this order:

```text
[thumb flexion, thumb yaw, index, middle, ring, little]
```

Each value is clamped to `0-255`. A value near `255` maps to the open end of the URDF joint range, while a value near `0` maps to the flexed end. The default open pose is:

```text
[200, 255, 255, 255, 255, 255]
```

The mapped maximum joint angles are `0.58 rad` for thumb flexion, `1.36 rad` for thumb yaw, and `1.6 rad` for each finger.

Generated joint names are prefixed by side, for example:

```text
left_lh_thumb_cmc_pitch
right_rh_thumb_cmc_pitch
```

## 📡 Joint-State Mapper

Run the mapper directly:

```bash
ros2 run hands_description o6_joint_state_mapper_node --ros-args \
  -p hand:=both \
  -p left_input_topic:=/openarmx/o6/left/command \
  -p right_input_topic:=/openarmx/o6/right/command \
  -p joint_states_topic:=/joint_states \
  -p publish_rate:=50.0
```

| Parameter | Default | Description |
|-----------|---------|-------------|
| `hand` | `both` | `left`, `right`, or `both` |
| `left_input_topic` | `/openarmx/o6/left/command` | Left six-value input |
| `right_input_topic` | `/openarmx/o6/right/command` | Right six-value input |
| `joint_states_topic` | `/joint_states` | Mapped URDF joint-state output |
| `publish_rate` | `50.0` | Output rate in Hz |

Input and output messages use `sensor_msgs/msg/JointState`. The mapper initializes each selected hand to the open pose and continues publishing the latest mapped state.

## 🧱 Robot Description

Generate a dual-hand URDF from the command line:

```bash
ros2 run hands_description o6_robot_description_cli \
  "$(ros2 pkg prefix hands_description)/share/hands_description" both
```

The last argument can be `left`, `right`, or `both`. The standalone dual-hand model places the left hand at `x=0.20 m` with a Z rotation of π and the right hand at `x=-0.20 m`.

The description node accepts:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `package_share` | empty | Installed `hands_description` share directory |
| `hand` | `both` | Model selection |

## 💻 C++ API

The exported `hands_description::o6_description_core` target provides:

```cpp
#include "openarmx_linker_o6_c/description/joint_mapping.hpp"
#include "openarmx_linker_o6_c/description/robot_description.hpp"
```

Core functions include `normalize_range()`, `range_to_arc()`, `o6_urdf_joint_positions()`, `prefixed_o6_joint_names()`, and `build_robot_description()`.

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
