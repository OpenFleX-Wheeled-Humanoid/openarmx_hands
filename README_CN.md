# 轮臂灵巧手使用教程
[English](README.md) | **中文**

## 终端 1

### 启动共总线真机（默认）

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  control_mode:=mit \
  robot_controller:=forward_position_controller \
  use_fake_hardware:=false \
  enable_forward_effort:=true
```

### 启动独立 O6 真机

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

### 仿真：

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  use_fake_hardware:=true \
  use_rviz:=true
```

## 终端 2

### 启动 VR

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_vr_teleop.launch.py vr_chassis:=true
```


## 终端 4

### 启动 HIGVR 手套读取(有摇杆部件)

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_joystick.launch.py
```

## 🧪 查看数据

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

## 终端 5

### HIGVR 到 O6 的桥接

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

# 双臂灵巧手使用教程

## 终端 1

### 启动共总线真机（默认）

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

### 启动独立 O6 真机

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=independent \
  right_o6_can_interface:=can2 \
  left_o6_can_interface:=can3 \
  control_mode:=mit \
  robot_controller:=forward_position_controller
```

### 仿真：

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=shared_bus \
  control_mode:=mit \
  robot_controller:=forward_position_controller \
  use_fake_hardware:=true
```

## 终端 2

### 启动 pico 桥接

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 run openarmx_teleop_bridge_vr openarmx_teleop_bridge_vr_node
```

## 终端 3

### 启动 ik 逆解

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_teleop_vr teleop_vr.launch.py controller_pose_mode:="hand"
```

## 终端 4

### 启动 HIGVR 手套读取

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
```

## 🧪 查看数据

```bash
ros2 topic echo /higvr/left_hand
ros2 topic echo /higvr/right_hand
ros2 topic echo /higvr/hand_frame
```

## 终端 5

### HIGVR 到 O6 的桥接

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

完整的 HIGVR 到 O6 控制流程和故障排查参阅 [`src/openarmx_hand_control/README_CN.md`](src/openarmx_hand_control/README_CN.md)。

---

## 作者 

- **OpenArmX Team**
- 公司：成都长数机器人有限公司
- 网站：<https://openarmx.com/>

## 版本

v0.1.0

## 许可证

本作品采用知识共享 署名-非商业性使用-相同方式共享 4.0 国际许可协议（CC BY-NC-SA 4.0）进行许可。

版权所有 (c) 2026 成都长数机器人有限公司 (Chengdu Changshu Robot Co., Ltd.)

详情请参阅 [LICENSE](LICENSE) 文件或访问：<http://creativecommons.org/licenses/by-nc-sa/4.0/>

---

## 联系我们

### 成都长数机器人有限公司

**Chengdu Changshu Robotics Co., Ltd.**

| 联系方式 | 信息 |
|---------|------|
| 邮箱 | [openarmrobot@gmail.com](mailto:openarmrobot@gmail.com) |
| 电话/微信 | +86-17746530375 |
| 官网 | <https://openarmx.com/> |
| 地址 | 天津经济技术开发区西区新业八街11号华诚机械厂 |
| 联系人 | 王先生 |
