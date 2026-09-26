## openarmx_hands_bridge Overview

English | [中文](README_CN.md)

`openarmx_hands_bridge` maps six HIGVR data-glove motion values to LinkerHand O6 commands. The node supports the left hand, right hand, or both hands and provides launch parameters for topics, publish rate, and mapping ranges.

## 🔄 Data Flow

The input message type is `std_msgs/msg/Int32MultiArray`:

| Input Topic | Default Order |
|-------------|---------------|
| `/higvr/left_hand` | `[thumb, index, middle, ring, little, thumb yaw]` |
| `/higvr/right_hand` | `[thumb, index, middle, ring, little, thumb yaw]` |

The output message type is `sensor_msgs/msg/JointState`:

| Output Topic | `position` Order |
|--------------|------------------|
| `/openarmx/o6/left/command` | `[thumb flexion, thumb yaw, index, middle, ring, little]` |
| `/openarmx/o6/right/command` | `[thumb flexion, thumb yaw, index, middle, ring, little]` |

## 📐 Default Mapping

HIGVR values range from `0-100`, while O6 commands range from `0-255`. A HIGVR value of `100` is near straight, and an O6 value of `255` is near open.

| HIGVR Input | O6 Output | Default Mapping |
|-------------|------------|-----------------|
| Thumb flexion | `position[0]` | `0-100` → `0-200` |
| Index flexion | `position[2]` | `0-100` → `0-255` |
| Middle flexion | `position[3]` | `0-100` → `0-255` |
| Ring flexion | `position[4]` | `0-100` → `0-255` |
| Little-finger flexion | `position[5]` | `0-100` → `0-255` |
| Thumb yaw | `position[1]` | `0-100` → `0-255` |

The default thumb-yaw mapping is therefore:

```text
HIGVR 0   -> O6 position[1] 0
HIGVR 100 -> O6 position[1] 255
```

## 🛠️ Build

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install --packages-select openarmx_hands_bridge
source install/setup.bash
```

## 🚀 Launch

Start the HIGVR driver, O6, and the bridge in order:

```bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

For hardware control, replace the O6 command with:

```bash
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both left_can:=can0 right_can:=can1
```

To bridge one hand only:

```bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=left
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=right
```

## ⚙️ Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `hand` | `both` | `left`, `right`, or `both` |
| `publish_rate` | `100.0` | O6 command publish rate |
| `left_input_topic` | `/higvr/left_hand` | Left HIGVR input |
| `right_input_topic` | `/higvr/right_hand` | Right HIGVR input |
| `left_output_topic` | `/openarmx/o6/left/command` | Left O6 output |
| `right_output_topic` | `/openarmx/o6/right/command` | Right O6 output |
| `thumb_close_value` | `0.0` | Thumb output at maximum flexion |
| `finger_close_value` | `0.0` | Other finger output at maximum flexion |
| `thumb_yaw_min` | `0.0` | Thumb-yaw output for input `0` |
| `thumb_yaw_max` | `255.0` | Thumb-yaw output for input `100` |
| `pinky_open_value` | `255.0` | Little-finger output when straight |

To reverse the thumb-yaw direction, swap its limits:

```bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py \
  thumb_yaw_min:=255.0 thumb_yaw_max:=0.0
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
