## hands_bringup 简介

[English](README.md) | 中文

`hands_bringup` 是用于单独启动一只或两只 LinkerHand O6 的 ROS 2 启动包，负责协调 O6 硬件驱动、手部模型、关节状态映射、`robot_state_publisher` 和 RViz。

该包只启动手部。需要同时启动双臂机器人和 O6 时，应使用 `openarmx_hand_bringup`。

## 🧩 主要文件

| 路径 | 说明 |
|------|------|
| `launch/o6_bringup.launch.py` | 对外启动入口 |
| `bringup/o6_launch_common.py` | 启动参数和节点构造逻辑 |

## 🛠️ 构建

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to hands_bringup
source install/setup.bash
```

## 🚀 仿真

```bash
# 双手
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both

# 单手
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=left
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=right
```

仿真模式不会启动 `o6_driver_node`，关节状态映射节点直接读取 `/openarmx/o6/left/command` 和 `/openarmx/o6/right/command`。

## 🔌 CAN 真机

```bash
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both left_can:=can0 right_can:=can1
```

独立启动的默认配置为左手 `can0`、ID `40`，右手 `can1`、ID `39`。启动前需要以 `1000000` bit/s 启用对应 CAN 接口。

指定接口启动单手：

```bash
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=left left_can:=can0 left_can_id:=40
```

## 🔗 RS485 真机

```bash
# 右手
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=right transport:=rs485 \
  right_modbus_port:=/dev/ttyUSB0 right_modbus_id:=39

# 双手
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both transport:=rs485 \
  left_modbus_port:=/dev/ttyUSB0 right_modbus_port:=/dev/ttyUSB1
```

默认 Modbus 波特率为 `115200`。

## ⚙️ 启动参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `sim` | `true` | 不启动硬件驱动，直接使用命令话题 |
| `hand` | `both` | `left`、`right` 或 `both` |
| `left_can` | `can0` | 左手 CAN 接口 |
| `right_can` | `can1` | 右手 CAN 接口 |
| `left_can_id` | `40` | 左手 CAN ID |
| `right_can_id` | `39` | 右手 CAN ID |
| `transport` | `can` | `can` 或 `rs485` |
| `left_modbus_port` | `/dev/ttyUSB0` | 左手 RS485 设备 |
| `right_modbus_port` | `/dev/ttyUSB1` | 右手 RS485 设备 |
| `left_modbus_id` | `40` | 左手 Modbus ID |
| `right_modbus_id` | `39` | 右手 Modbus ID |
| `modbus_baudrate` | `115200` | RS485 波特率 |
| `driver_rate` | `100.0` | 硬件查询与状态发布频率 |
| `publish_rate` | `50.0` | URDF 关节状态映射频率 |
| `info_rate` | `1.0` | 诊断和设备信息发布频率 |
| `poll_touch` | `false` | 查询普通压感数据 |
| `poll_matrix_touch` | `false` | 查询矩阵压感数据 |
| `poll_diagnostics` | `true` | 查询温度、电流和故障 |
| `poll_device_info` | `true` | 查询设备信息 |
| `launch_rviz` | `true` | 是否启动 RViz |

## 📡 节点流程

真机模式下：

```text
/openarmx/o6/*/command -> hands_hardware/o6_driver_node -> O6 真机
O6 真机 -> /openarmx/o6/*/state -> o6_joint_state_mapper_node -> /joint_states
```

`robot_state_publisher` 使用映射后的关节状态，RViz 显示独立手部模型。

## 许可证

本作品采用知识共享 署名-非商业性使用-相同方式共享 4.0 国际许可协议 (CC BY-NC-SA 4.0) 进行许可。

版权所有 (c) 2026 成都长数机器人有限公司 (Chengdu Changshu Robot Co., Ltd.)

详情请访问：http://creativecommons.org/licenses/by-nc-sa/4.0/

## 作者

- **OpenArmX 团队**
- 公司: 成都长数机器人有限公司
- 网站: https://openarmx.com/

## 版本

**当前版本**：0.0.0

## 致谢

本包是 OpenArmX 机器人平台生态系统的一部分，专为协作机器人领域的研究和工业应用而开发。

---

## 📞 联系我们

### 成都长数机器人有限公司

**Chengdu Changshu Robotics Co., Ltd.**

| 联系方式 | 信息 |
|----------|------|
| 📧 邮箱 | openarmrobot@gmail.com |
| 📱 电话/微信 | +86-17746530375 |
| 🌐 官网 | <https://openarmx.com/> |
| 📍 地址 | 天津经济技术开发区西区新业八街11号华诚机械厂 |
| 👤 联系人 | 王先生 |
