## openarmx_hand_control 简介

[English](README.md) | 中文

`openarmx_hand_control` 包含将 HIGVR 串口数据手套接入 OpenArmX LinkerHand O6 灵巧手的 ROS 2 软件包。

该目录是软件包集合，本身不是独立 ROS 2 包。需要使用 `colcon` 编译其下的两个子包。

## 包含的软件包

| 软件包 | 职责 | 详细文档 |
|--------|------|----------|
| `openarmx_hands_hig` | 读取并解析 HIGVR 串口手套数据 | [README_CN](openarmx_hands_hig/README_CN.md) |
| `openarmx_hands_bridge` | 将 HIGVR 数值映射为 O6 控制命令 | [README_CN](openarmx_hands_bridge/README_CN.md) |

## 控制流程

```text
HIGVR 串口接收器
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
                    openarmx_hand_bringup 提供的 O6 驱动
                                      │
                                      ▼
                              左右 O6 灵巧手
```

`openarmx_hands_hig` 只负责读取手套，`openarmx_hands_bridge` 只负责数值映射。O6 真机或仿真驱动由 `openarmx_hand_bringup` 单独提供。

## 编译

在工作空间根目录执行：

```bash
cd ~/openflex_all/openflex_ws
colcon build --symlink-install --packages-select \
  openarmx_hands_hig \
  openarmx_hands_bridge
source install/setup.bash
```

需要同时编译完整 O6 控制链时执行：

```bash
colcon build --symlink-install --packages-up-to \
  openarmx_hand_bringup \
  openarmx_hands_hig \
  openarmx_hands_bridge
source install/setup.bash
```

## 双手完整启动流程

以下每个进程应在已经加载工作空间环境的独立终端中运行。

### 1. 启动机械臂和 O6

真机：

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  control_mode:=mit \
  robot_controller:=forward_position_controller \
  use_fake_hardware:=false \
  enable_forward_effort:=true
```

仿真：

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_integrated_bringup integrated_robot_o6_bringup.launch.py \
  use_fake_hardware:=true \
  use_rviz:=true
```

真机默认映射为右手 O6 `can2/0x27`、左手 O6 `can3/0x28`。

### 2. 启动 HIGVR 输入

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
```

### 3. 启动 HIGVR 到 O6 桥接

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

桥接节点默认以 `100 Hz` 持续发布。接收到有效手套数据后，映射值会发送到 O6 控制话题。

## 单手模式

左手：

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_left.launch.py
```

```
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=left
```

右手：

```bash
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_hig higvr_right.launch.py
```
```
cd ~/openflex_all/openflex_ws
source install/setup.bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=right
```

## 检查数据链

```bash
ros2 topic echo /higvr/left_hand
ros2 topic echo /higvr/right_hand
ros2 topic echo /openarmx/o6/left/command
ros2 topic echo /openarmx/o6/right/command
```

检查话题类型和连接情况：

```bash
ros2 topic info /higvr/left_hand --verbose
ros2 topic info /openarmx/o6/left/command --verbose
```

## 默认数据映射

HIGVR 发布的六个整数顺序为：

```text
[拇指弯曲, 食指, 中指, 无名指, 小指, 拇指横摆]
```

桥接节点将 HIGVR 的 `0-100` 范围转换为 O6 控制值，并按照以下顺序发布：

```text
[拇指弯曲, 拇指横摆, 食指, 中指, 无名指, 小指]
```

O6 数值会限制在 `0-255`。详细映射范围和反向配置参阅 [openarmx_hands_bridge/README_CN.md](openarmx_hands_bridge/README_CN.md)。

## 串口说明

HIGVR 驱动默认使用 `115200` 波特率，可以自动发现 `/dev/higvr_glove`、`/dev/serial/by-id/*`、`/dev/ttyACM*` 和 `/dev/ttyUSB*`。需要持久串口权限时，将当前用户加入 `dialout` 组并重新登录：

```bash
sudo usermod -aG dialout $USER
```

## 安全与故障排查

- 控制真机前，先在仿真环境中确认 O6 命令方向和范围。
- 第一次真机测试时保证急停按钮随时可用。
- HIGVR 话题没有数据时，检查串口设备、权限、波特率和 `hand_mode`。
- HIGVR 话题更新但 O6 命令不更新时，检查 `openarmx_hands_bridge` 是否以匹配的 `hand` 参数运行。
- O6 命令话题更新但灵巧手不运动时，检查 O6 驱动、CAN 接口、CAN ID 和 bringup 模式。

## 许可证

本作品采用知识共享 署名-非商业性使用-相同方式共享 4.0 国际许可协议（CC BY-NC-SA 4.0）。

版权所有 (c) 2026 成都长数机器人有限公司（Chengdu Changshu Robot Co., Ltd.）

详情请参阅 [LICENSE](LICENSE) 或访问：http://creativecommons.org/licenses/by-nc-sa/4.0/

## 作者

- **OpenArmX 团队**
- 公司：成都长数机器人有限公司
- 网站：https://openarmx.com/

## 版本

**当前版本**：0.1.0
