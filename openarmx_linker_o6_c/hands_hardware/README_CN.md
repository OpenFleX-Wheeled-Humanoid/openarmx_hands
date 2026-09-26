## hands_hardware 简介

[English](README.md) | 中文

`hands_hardware` 提供 LinkerHand O6 的 C++ 硬件层，包含 CAN 和 RS485 驱动、统一驱动接口以及 `o6_driver_node` ROS 2 可执行程序。

节点接收六路位置与设置命令，与一只 O6 通信，并发布状态、诊断、压感和设备信息。

## 🧩 组成

| 组件 | 说明 |
|------|------|
| `o6_hardware_core` | 导出的 C++ CAN/RS485 驱动库 |
| `O6DriverBase` | 统一驱动接口 |
| `O6CanDriver` | SocketCAN 实现 |
| `O6Rs485Driver` | 串口 Modbus 风格实现 |
| `o6_driver_node` | 单只左手或右手 O6 的 ROS 2 封装节点 |

## 🛠️ 构建

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to hands_hardware
source install/setup.bash
```

## 🤖 命令格式

位置、速度和力矩数组均使用以下六路顺序：

```text
[拇指弯曲, 拇指横摆, 食指, 中指, 无名指, 小指]
```

数值会四舍五入并限制到 `0-255`。位置命令缺少的通道使用张开姿态补齐；速度和力矩缺少的通道使用对应命令的默认值。

节点启动时会向六个通道发送速度 `255`、力矩 `255` 和张开姿态 `[200, 255, 255, 255, 255, 255]`。

## 🔌 使用 CAN 运行

先以 `1000000` bit/s 启用 CAN 接口，再运行一个驱动节点：

```bash
ros2 run hands_hardware o6_driver_node --ros-args \
  -p side:=right \
  -p transport:=can \
  -p can:=can2 \
  -p can_id:=39
```

左手需要再启动一个节点，使用 `side:=left`、对应 CAN 接口和 ID `40`。

## 🔗 使用 RS485 运行

```bash
ros2 run hands_hardware o6_driver_node --ros-args \
  -p side:=right \
  -p transport:=rs485 \
  -p modbus_port:=/dev/ttyUSB0 \
  -p modbus_id:=39 \
  -p modbus_baudrate:=115200
```

RS485 实现支持位置、速度、力矩、当前位置、已设置速度/力矩、温度和故障寄存器。当该传输方式不提供某些高级传感数据时，相应字段会返回占位值。

## ⚙️ 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `side` | `right` | 话题侧别：`left` 或 `right` |
| `transport` | `can` | `can` 或 `rs485` |
| `can` | `can0` | SocketCAN 接口 |
| `can_id` | `39` | CAN 设备 ID |
| `modbus_port` | `/dev/ttyUSB0` | RS485 串口设备 |
| `modbus_id` | `39` | RS485 设备 ID |
| `modbus_baudrate` | `115200` | 串口波特率 |
| `publish_rate` | `100.0` | 位置查询和状态发布频率 |
| `info_rate` | `1.0` | 信息与诊断发布频率 |
| `init_speed` | 六个 `255` | 启动速度值 |
| `init_torque` | 六个 `255` | 启动力矩值 |
| `poll_touch` | `false` | 查询普通压感数据 |
| `poll_matrix_touch` | `false` | 查询矩阵压感数据 |
| `poll_diagnostics` | `true` | 查询温度、电流和故障 |
| `poll_device_info` | `true` | 查询设备信息 |

## 📥 订阅话题

`side` 参数决定话题前缀 `/openarmx/o6/<side>`。

| 话题后缀 | 消息类型 | 用途 |
|----------|----------|------|
| `/command` | `sensor_msgs/msg/JointState` | `position` 中的六路位置命令 |
| `/setting_cmd` | `std_msgs/msg/String` | JSON 速度或力矩设置 |

设置示例：

```json
{"setting_cmd":"set_speed","params":{"speed":[255,255,255,255,255,255]}}
```

```json
{"setting_cmd":"set_torque","params":{"torque":[200,200,200,200,200,200]}}
```

## 📤 发布话题

| 话题后缀 | 消息类型 | 用途 |
|----------|----------|------|
| `/state` | `sensor_msgs/msg/JointState` | 当前六路手部状态 |
| `/info` | `std_msgs/msg/String` | 组合设备与诊断信息 JSON |
| `/temperature` | `std_msgs/msg/Float32MultiArray` | 各通道温度 |
| `/current` | `std_msgs/msg/Float32MultiArray` | 各通道电流 |
| `/fault` | `std_msgs/msg/Int32MultiArray` | 各通道故障值 |
| `/device_info` | `std_msgs/msg/String` | 设备信息 JSON |
| `/force` | `std_msgs/msg/Float32MultiArray` | 普通压感数据 |
| `/matrix_touch` | `std_msgs/msg/String` | 矩阵压感 JSON |
| `/matrix_touch_mass` | `std_msgs/msg/String` | 矩阵压感中心/质量 JSON |
| `/matrix_touch_pc` | `sensor_msgs/msg/PointCloud2` | 点云形式的矩阵压感值 |

只有启用对应查询参数时才会读取压感话题。如果读取当前位置时发生异常，节点会继续发布最近一次有效状态并输出警告。

## 💻 C++ 接口

导出的 `hands_hardware::o6_hardware_core` 目标提供：

```cpp
#include "openarmx_linker_o6_c/drivers/can_driver.hpp"
#include "openarmx_linker_o6_c/drivers/rs485_driver.hpp"
```

两种实现均遵循 `O6DriverBase`，该接口定义位置、速度、力矩、状态、诊断、压感和设备信息操作。

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
