## openarmx_linker_o6_c 简介

[English](README.md) | 中文

`openarmx_linker_o6_c` 是 LinkerHand O6 的 ROS 2 功能集合，包含三个独立软件包：

| 软件包 | 说明 |
|--------|------|
| `hands_hardware` | CAN/RS485 驱动和 O6 硬件节点 |
| `hands_description` | 左右手 URDF、RViz 配置和关节状态映射 |
| `hands_bringup` | 仿真与真机的统一启动文件 |

O6 命令的顺序为 `[拇指弯曲, 拇指横摆, 食指, 中指, 无名指, 小指]`，取值范围为 `0-255`。`255` 约为张开，`0` 约为闭合；默认张开姿态为 `[200, 255, 255, 255, 255, 255]`。

## 🛠️ 构建

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install \
  --packages-select hands_description hands_hardware hands_bringup
source install/setup.bash
```

## 🚀 启动

### 仿真与 RViz

```bash
# 双手
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both

# 单手
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=left
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=right
```

### CAN 真机

```bash
# 双手
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both left_can:=can0 right_can:=can1

# 左手
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=left left_can:=can0

# 右手
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=right right_can:=can1
```

默认 CAN ID 为左手 `40`、右手 `39`。可用 `left_can_id` 和 `right_can_id` 覆盖。

CAN 接口未启动时先执行：

```bash
sudo ip link set can0 down
sudo ip link set can0 up type can bitrate 1000000
```

### RS485 真机

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

默认 Modbus 波特率为 `115200`，可用 `modbus_baudrate` 覆盖。

## ⚙️ 主要参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `sim` | `true` | `true` 使用模型，`false` 启动真机驱动 |
| `hand` | `both` | `left`、`right` 或 `both` |
| `transport` | `can` | `can` 或 `rs485` |
| `launch_rviz` | `true` | 是否启动 RViz |
| `driver_rate` | `100.0` | 真机驱动循环频率 |
| `publish_rate` | `50.0` | 关节状态映射发布频率 |
| `info_rate` | `1.0` | 信息和诊断话题发布频率 |
| `poll_diagnostics` | `true` | 查询温度、电流和故障状态 |
| `poll_device_info` | `true` | 查询设备信息 |
| `poll_touch` | `false` | 发布 `/force` 普通压感数据 |
| `poll_matrix_touch` | `false` | 发布矩阵压感相关话题 |

## 🧤 配合 HIGVR 手套

依次启动手套读取、O6 和桥接：

```bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

将第二条命令改为 `sim:=false` 并配置 CAN 参数即可控制真机。

## 🖥️ 手动控制 GUI

先启动 O6 仿真或真机，再运行：

```bash
ros2 run openarmx_hand_gui o6_manual_control_gui
```

GUI 会向 `/openarmx/o6/left/command`、`/openarmx/o6/right/command` 以及对应的 `setting_cmd` 话题发布消息。

## 许可证

本作品采用知识共享 署名-非商业性使用-相同方式共享 4.0 国际许可协议 (CC BY-NC-SA 4.0) 进行许可。

版权所有 (c) 2026 成都长数机器人有限公司 (Chengdu Changshu Robot Co., Ltd.)

详情请参阅 [LICENSE](LICENSE) 或访问：http://creativecommons.org/licenses/by-nc-sa/4.0/

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
