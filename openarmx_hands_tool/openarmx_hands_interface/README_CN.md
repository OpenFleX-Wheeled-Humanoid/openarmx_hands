## openarmx_hands_interface 简介

[English](README.md) | 中文

`openarmx_hands_interface` 提供物理 SocketCAN 接口管理和硬件状态诊断工具，适用于安装了两只 LinkerHand O6 灵巧手的 OpenArmX 双臂机器人。

这个目录是直接从源码运行的工具集合，并不是独立的 ROS 2 包。它没有 `package.xml` 或构建配置，不参与 `colcon build`，需要直接使用 Python 运行。

## 主要文件

| 文件 | 说明 |
|------|------|
| `can_up.py` | 启动检测到的物理 CAN 接口 |
| `can_down.py` | 关闭检测到的物理 CAN 接口 |
| `check_hand_status.py` | 不启动 ROS 2 驱动，直接查询左右 O6 灵巧手 |
| `check_motor_status.py` | 独立 CAN 模式下检查双七轴机械臂和左右 O6 灵巧手 |
| `check_motor_status_shared_bus.py` | 默认共总线模式下检查双七轴机械臂和左右 O6 灵巧手 |

## 环境要求

- 支持 SocketCAN 的 Linux 系统
- Python 3
- `iproute2`（提供 `ip link`）
- 物理 CAN 适配器及其内核驱动
- 两个整机状态脚本的机械臂部分需要 `openarmx_arm_driver`

CAN 配置脚本会执行 `sudo ip link`，终端可能要求输入系统密码。

## 硬件映射

默认共总线模式：

| 设备 | CAN 接口 | CAN ID |
|------|----------|--------|
| 右臂 | `can0` | 电机 ID `1-7` |
| 右手 O6 | `can0` | `0x27`（`39`） |
| 左臂 | `can1` | 电机 ID `1-7` |
| 左手 O6 | `can1` | `0x28`（`40`） |

独立 CAN 模式：

| 设备 | CAN 接口 | CAN ID |
|------|----------|--------|
| 右臂 | `can0` | 电机 ID `1-7` |
| 左臂 | `can1` | 电机 ID `1-7` |
| 右手 O6 | `can2` | `0x27`（`39`） |
| 左手 O6 | `can3` | `0x28`（`40`） |

整机检查脚本不会查询已经移除的夹爪电机 ID `8`。

## 进入工具目录

```bash
cd ~/openarmx/openarmx_ws/src/openarmx_hands/src/openarmx_hands_tool
```

## CAN 接口管理

以默认波特率 `1000000` 启动所有检测到的物理 CAN 接口：

```bash
python3 openarmx_hands_interface/can_up.py
```

指定接口、波特率或回环模式：

```bash
python3 openarmx_hands_interface/can_up.py can0 can1 can2 can3
python3 openarmx_hands_interface/can_up.py can2 can3 --bitrate 1000000
python3 openarmx_hands_interface/can_up.py can2 --loopback
```

关闭所有已启动的物理 CAN 接口或指定接口：

```bash
python3 openarmx_hands_interface/can_down.py
python3 openarmx_hands_interface/can_down.py can2 can3
```

脚本通过 `/sys/class/net` 识别物理 CAN：接口必须是 Linux 网络类型 `280`，并且存在硬件 `device` 项。虚拟 CAN 接口会被主动排除。

## 只检查 O6 灵巧手

在所有已启动的物理 CAN 接口上探测默认的左右手 O6 ID：

```bash
python3 openarmx_hands_interface/check_hand_status.py
```

限制扫描接口，或者修改 ID 和超时时间：

```bash
python3 openarmx_hands_interface/check_hand_status.py can2 can3 --show-missing
python3 openarmx_hands_interface/check_hand_status.py can2 \
  --right-id 0x27 --left-id 0x28 --timeout 0.3
```

每只检测到的灵巧手会显示六个 O6 通道：

- 拇指（`thumb_cmc_pitch`）
- 虎口（`thumb_cmc_yaw`）
- 食指（`index_mcp_pitch`）
- 中指（`middle_mcp_pitch`）
- 无名指（`ring_mcp_pitch`）
- 小指（`pinky_mcp_pitch`）

位置、速度、力矩、温度、故障和电流显示为 O6 返回的原始字节值。脚本还会查询固件版本和设备序列号。

## 检查完整机器人

使用默认共总线映射检查双臂和两只 O6 灵巧手：

```bash
python3 openarmx_hands_interface/check_motor_status_shared_bus.py
```

需要覆盖共总线接口时：

```bash
python3 openarmx_hands_interface/check_motor_status_shared_bus.py \
  --right-can can0 --left-can can1 \
  --right-hand-id 0x27 --left-hand-id 0x28
```

使用独立 CAN 映射检查双臂和两只 O6 灵巧手：

```bash
python3 openarmx_hands_interface/check_motor_status.py
```

机械臂部分明确只查询每条手臂的电机 ID `1-7`，不会查询电机 ID `8`，也不会使能电机、切换模式或发送运动目标。O6 部分复用 `check_hand_status.py` 的只读查询协议。

需要时可以覆盖接口、ID 和超时时间：

```bash
python3 openarmx_hands_interface/check_motor_status.py \
  --right-arm-can can0 --left-arm-can can1 \
  --right-hand-can can2 --left-hand-can can3 \
  --right-hand-id 0x27 --left-hand-id 0x28 \
  --arm-timeout 1.0 --hand-timeout 0.2
```

最终汇总会显示 `14` 个机械臂电机和 `2` 只 O6 灵巧手的响应数量。只有预期的 16 个设备全部响应时，命令才会成功退出。

## 退出状态

| 脚本 | 成功条件 |
|------|----------|
| `can_up.py` | 所有请求的 CAN 配置命令执行完成 |
| `can_down.py` | 所有请求的 CAN 关闭命令执行完成 |
| `check_hand_status.py` | 至少检测到一只 O6 灵巧手 |
| `check_motor_status.py` | 14 个机械臂电机和两只 O6 灵巧手全部响应 |
| `check_motor_status_shared_bus.py` | 14 个机械臂电机和两只 O6 灵巧手全部响应 |

三个状态检查脚本通过 `Ctrl+C` 中断时返回状态码 `130`。

## 安全与故障排查

- 这些工具用于真实 CAN 硬件，不适用于仿真环境。
- 运行状态检查脚本前先执行 `can_up.py`。
- 状态检查会发送查询帧，但不会发送运动命令。
- 进行底层诊断前，建议停止正常的机械臂和 O6 驱动，避免轮询流量重叠。
- 使用 `ip -details link show type can` 检查接口状态和波特率。
- 如果整机检查无法导入 `openarmx_arm_driver`，请安装 `openarmx_motor_manager` 使用的对应依赖。

## 许可证

本作品采用知识共享 署名-非商业性使用-相同方式共享 4.0 国际许可协议（CC BY-NC-SA 4.0）。

版权所有 (c) 2026 成都长数机器人有限公司（Chengdu Changshu Robot Co., Ltd.）

详情请参阅 [LICENSE](../LICENSE) 或访问：http://creativecommons.org/licenses/by-nc-sa/4.0/

## 作者

- **OpenArmX 团队**
- 公司：成都长数机器人有限公司
- 网站：https://openarmx.com/

## 版本

**当前版本**：0.1.0
