## openarmx_hands_tool 简介

[English](README.md) | 中文

`openarmx_hands_tool` 提供 OpenArmX 手部设备的辅助工具，包括 O6 手动控制 GUI、CAN 接口启停脚本和电机状态查询脚本。

## 🧩 工具列表

| 工具 | 说明 |
|------|------|
| `openarmx_hand_gui` | ROS 2 O6 手动控制 GUI 软件包 |
| `openarmx_hands_interface/can_up.py` | 启动真实 CAN 接口 |
| `openarmx_hands_interface/can_down.py` | 关闭真实 CAN 接口 |
| `openarmx_hands_interface/check_hand_status.py` | 查询 O6 位置、速度、力矩、温度、故障和电流 |
| `openarmx_hands_interface/check_motor_status.py` | 独立 CAN 模式下检查双臂和左右 O6 灵巧手 |
| `openarmx_hands_interface/check_motor_status_shared_bus.py` | 默认共总线模式下检查双臂和左右 O6 灵巧手 |

## 🛠️ 构建 GUI

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install --packages-select openarmx_hand_gui
source install/setup.bash
```

运行 GUI：

```bash
ros2 run openarmx_hand_gui o6_manual_control_gui
```

使用方法参阅 [`openarmx_hand_gui/README_CN.md`](openarmx_hand_gui/README_CN.md)。

CAN 配置和硬件状态诊断方法参阅 [`openarmx_hands_interface/README_CN.md`](openarmx_hands_interface/README_CN.md)。

## 🔌 启动 CAN

进入工具目录：

```bash
cd <openarmx_ws_hand>
cd src/openarmx_hands/src/openarmx_hands_tool
```

启动所有检测到的真实 CAN 接口，默认波特率为 `1000000`：

```bash
python3 openarmx_hands_interface/can_up.py
```

启动指定接口或指定波特率：

```bash
python3 openarmx_hands_interface/can_up.py can0
python3 openarmx_hands_interface/can_up.py can0 --bitrate 1000000
```

脚本内部使用 `sudo ip link`，终端可能提示输入密码。

## ⏹️ 关闭 CAN

```bash
# 关闭所有已启动的真实 CAN 接口
python3 openarmx_hands_interface/can_down.py

# 关闭指定接口
python3 openarmx_hands_interface/can_down.py can0
```

## 🧪 查询 O6 状态

查询所有已启动的真实 CAN 接口：

```bash
python3 openarmx_hands_interface/check_hand_status.py
```

查询指定接口并显示未响应的探测项：

```bash
python3 openarmx_hands_interface/check_hand_status.py can2 can3 --show-missing
```

默认右手 ID 为 `0x27`（十进制 `39`），左手 ID 为 `0x28`（十进制 `40`）。自定义 ID 和超时时间：

```bash
python3 openarmx_hands_interface/check_hand_status.py can2 \
  --right-id 0x27 --left-id 0x28 --timeout 0.3
```

## 🧪 查询整机状态

默认共总线模式下，右臂与右手共用 `can0`，左臂与左手共用 `can1`：

```bash
python3 openarmx_hands_interface/check_motor_status_shared_bus.py
```

独立 CAN 模式下检查两条七轴机械臂和两只 O6 灵巧手：

```bash
python3 openarmx_hands_interface/check_motor_status.py
```

默认映射为右臂 `can0`、左臂 `can1`、右手 O6 `can2/0x27`、左手 O6 `can3/0x28`。机械臂部分明确只查询电机 ID `1-7`，不会查询已经移除的夹爪电机 ID `8`。

需要时可以覆盖 CAN 接口：

```bash
python3 openarmx_hands_interface/check_motor_status.py \
  --right-arm-can can0 --left-arm-can can1 \
  --right-hand-can can2 --left-hand-can can3
```

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
