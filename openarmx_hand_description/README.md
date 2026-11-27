## openarmx_hand_description Overview

English | [中文](README_CN.md)

`openarmx_hand_description` is a ROS 2 Python model-composition package for an OpenArmX dual V10 arm robot equipped with two LinkerHand O6 hands. It builds a complete URDF at runtime while leaving the upstream `openarmx_description` and `hands_description` packages unchanged.

This package is a Python library used by `openarmx_hand_bringup`; it does not provide a standalone ROS 2 node or launch file.

## 🧩 Main Files

| Path | Description |
|------|-------------|
| `openarmx_hand_description/model.py` | Loads, patches, and combines the arm and O6 models |
| `config/model.yaml` | Link 7, TCP, and O6 mounting configuration |
| `meshes/arm/v10/visual/` | Replacement visual mesh for arm link 7 |
| `meshes/arm/v10/collision/` | Replacement collision mesh for arm link 7 |

## 🏗️ Model Composition

`build_robot_description()` performs the following steps:

1. Renders the upstream `openarmx_description/urdf/robot/v10.urdf.xacro` as a bimanual V10 robot with ROS 2 control enabled and no upstream end effector.
2. Replaces the visual mesh, collision mesh, and inertial mass of each arm's `link7`.
3. Updates the left and right `link7_pico_joint` TCP transforms.
4. In `shared_bus` mode, replaces each ros2_control system with the combined arm/O6 plugin and adds six actuated O6 joints. In `independent` mode, retains the original seven-joint arm hardware systems.
5. Appends the prefixed left and right O6 models from `hands_description`.
6. Adds fixed joints connecting both O6 base links to the arm end links.
7. Injects `node_namespace` into each specialized hardware plugin when a namespace is provided.

The returned value is a complete URDF XML string used by both `ros2_control_node` and `robot_state_publisher`.

## ⚙️ Model Configuration

The defaults in `config/model.yaml` include:

| Setting | Left | Right |
|---------|------|-------|
| Link 7 mass | `0.65 kg` | `0.65 kg` |
| O6 parent link | `openarmx_left_link7` | `openarmx_right_link7` |
| O6 child link | `left_lh_hand_base_link` | `right_rh_hand_base_link` |
| Mount position | `0 0 0.05` | `0 0 0.05` |
| Mount rotation | `0 0 1.57079632679` | `0 0 -1.57079632679` |

The file also defines the replacement Link 7 meshes and the left/right Pico TCP transforms. Adjust these values only when the physical mounting geometry or end-link model changes.

## 🛠️ Build

From the ROS 2 workspace containing this repository:

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to openarmx_hand_description
source install/setup.bash
```

## 🐍 Python API

```python
from openarmx_hand_description import build_robot_description

robot_description = build_robot_description(
    o6_hardware_mode="shared_bus",
    use_fake_hardware="false",
    left_can_interface="can1",
    right_can_interface="can0",
    left_o6_can_id="0x28",
    right_o6_can_id="0x27",
    o6_update_rate="50.0",
    can_fd="false",
    control_mode="mit",
    node_namespace="",
)
```

Supported keyword mappings are passed to the upstream V10 Xacro. Common values are:

| Argument | Default | Description |
|----------|---------|-------------|
| `o6_hardware_mode` | `shared_bus` | Select `shared_bus` or `independent` |
| `use_fake_hardware` | `false` | Select simulated arm hardware |
| `left_can_interface` | `can1` | Left-arm CAN interface |
| `right_can_interface` | `can0` | Right-arm CAN interface |
| `left_o6_can_id` | `0x28` | Left O6 CAN ID |
| `right_o6_can_id` | `0x27` | Right O6 CAN ID |
| `o6_update_rate` | `50.0` | O6 asynchronous communication rate |
| `can_fd` | `false` | Enable CAN FD |
| `control_mode` | `mit` | Arm control mode |
| `node_namespace` | empty | Namespace injected into the specialized hardware plugins |

The composition always requests a bimanual V10 model with `ee_type=none`, `hand=false`, and `ros2_control=true`, then attaches O6 models itself. Both modes use identical geometry; only the ros2_control hardware boundary changes.

## 🔗 Bringup Integration

`openarmx_hand_bringup` calls this API once for the main robot description and again when gravity compensation is enabled. The same geometry and hardware mappings are therefore used by arm control, visualization, and gravity calculation.

Start the complete robot through the bringup package:

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py
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
