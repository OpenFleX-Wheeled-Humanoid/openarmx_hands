## openarmx_hands_hig 简介
[English](README.md) | 中文

`openarmx_hands_hig` 是用于读取 HIGVR 串口数据手套的 ROS 2 C++ 节点包，支持左手、右手和双手读取，可自动发现串口、处理临时权限并在断开后重连。

## 🧩 主要文件

| 路径 | 说明 |
|------|------|
| `src/higvr_hands_node.cpp` | 串口发现、协议解析、左右手识别和话题发布 |
| `src/higvr_hands_joystick.cpp` | 新版 12 列手套读取、校准及 VR 话题发布 |
| `launch/higvr_hands.launch.py` | 通用启动文件，通过 `hand_mode` 选择手部 |
| `launch/higvr_both.launch.py` | 双手启动文件 |
| `launch/higvr_left.launch.py` | 左手启动文件 |
| `launch/higvr_right.launch.py` | 右手启动文件 |

## 🔌 串口协议

- 默认波特率：`115200`
- 数据格式：以制表符分隔的 ASCII 文本，每帧以换行结束
- 每帧字段：`[左右手, 手指1, 手指2, 手指3, 手指4, 手指5, 手背开合]`
- 左右手字段：`1` 表示左手，`2` 表示右手
- 六路动作值范围：`0-100`，数值越小通常表示弯曲越大

节点不执行 `100 - value` 数值反转。为了统一左右手的手指语义顺序，默认会翻转左手前五路手指的排列，右手保持原始排列；第六路手背开合始终位于末尾。

## 📡 发布话题

| 话题 | 消息类型 | 内容 |
|------|----------|------|
| `/higvr/left_hand` | `std_msgs/msg/Int32MultiArray` | 左手六路动作值 |
| `/higvr/right_hand` | `std_msgs/msg/Int32MultiArray` | 右手六路动作值 |
| `/higvr/left_accessories` | `std_msgs/msg/Int32MultiArray` | `[X原始值, Y原始值, 摇杆按下, Y键, X键]` |
| `/higvr/right_accessories` | `std_msgs/msg/Int32MultiArray` | `[X原始值, Y原始值, 摇杆按下, A键, B键]` |
| `/higvr/hand_frame` | `std_msgs/msg/Int32MultiArray` | `[左右手, 六路动作值, 五路附件原始值]` |
| `/higvr/raw` | `std_msgs/msg/String` | 原始串口文本，默认关闭 |

当 `reverse_left_fingers:=true` 时，左手发布顺序为 `[手指5, 手指4, 手指3, 手指2, 手指1, 手背开合]`；右手默认顺序为 `[手指1, 手指2, 手指3, 手指4, 手指5, 手背开合]`。

## 🛠️ 构建

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install --packages-select openarmx_hands_hig
source install/setup.bash
```

## 🚀 启动

带摇杆的启动方式：

```bash
ros2 launch openarmx_hands_hig higvr_joystick.launch.py \
  hand_mode:=both port:=auto baud_rate:=115200
```

无摇杆的启动方式：
```bash
# 双手
ros2 launch openarmx_hands_hig higvr_both.launch.py

# 左手
ros2 launch openarmx_hands_hig higvr_left.launch.py

# 右手
ros2 launch openarmx_hands_hig higvr_right.launch.py

# 通用启动文件
ros2 launch openarmx_hands_hig higvr_hands.launch.py \
  hand_mode:=both port:=auto baud_rate:=115200
```

该节点读取 12 列数据帧。`/higvr/left_hand` 和 `/higvr/right_hand` 仍只发布原来的六路手部数据，`/higvr/hand_frame` 发布包含新增部件的完整原始帧。附件话题前两项统一表示 X、Y 原始轴值，第 3 项为摇杆按下，第 4、5 项分别为左手 Y/X 或右手 A/B。节点同时按固定中心和死区转换摇杆，并发布到 `/pico_*` VR 标准话题，按钮发布为 `Bool`。左右手标识默认分别为 `1` 和 `2`，可通过参数配置。

## ⚙️ 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `port` | `auto` | 串口设备路径或自动发现 |
| `baud_rate` | `115200` | 串口波特率 |
| `hand_mode` | `both` | `left`、`right` 或 `both` |
| `left_topic` | `/higvr/left_hand` | 左手话题 |
| `right_topic` | `/higvr/right_hand` | 右手话题 |
| `all_topic` | `/higvr/hand_frame` | 汇总帧话题 |
| `raw_topic` | `/higvr/raw` | 原始文本话题 |
| `publish_raw` | `false` | 是否发布原始文本 |
| `publish_vr_topics` | `true` | 是否发布 `/pico_*` VR 标准输入话题 |
| `joystick_center` | `510` | 摇杆 ADC 中心值 |
| `joystick_deadzone` | `100` | ADC 原始死区半径，中心 ±100 输出为 0 |
| `joystick_minimum` | `0` | 摇杆 ADC 下限 |
| `joystick_maximum` | `1023` | 摇杆 ADC 上限 |
| `joystick_invert_x` | `true` | X 轴取反，使左为负、右为正 |
| `joystick_invert_y` | `false` | Y 轴取反开关；默认上为正、下为负 |
| `joystick_invert_left_y` | `true` | 左手 Y 轴取反，使手套向前方向匹配底盘前进 |
| `joystick_invert_right_y` | `false` | 右手 Y 轴取反开关 |
| `reconnect_interval` | `1.0` | 断开后的重连间隔，单位为秒 |
| `auto_fix_permissions` | `true` | 权限不足时尝试执行 `sudo chmod a+rw` |
| `reverse_left_fingers` | `true` | 翻转左手前五路顺序 |
| `reverse_right_fingers` | `false` | 翻转右手前五路顺序 |

`/pico_*` 兼容话题遵循 VR 控制器的输入节奏：摇杆偏离中心时持续发布轴值，回到中心时只发布一次 `0.0`；X/Y/A 按键按住期间持续发布 `true`，松开时只发布一次 `false`，以满足升降台等下游节点的超时保护；摇杆按下按住期间持续发布 `true`，松开时只发布一次 `false`。右手 B 按下沿切换头控状态并发布新的状态值，松开本身不改变头控状态。串口断开时会补发安全的释放/回中状态。`/higvr/*` 原始话题仍按每帧数据发布。

## 🔍 自动发现与权限

当 `port:=auto` 时，节点按以下顺序查找设备：

```text
/dev/higvr_glove
/dev/serial/by-id/*
/dev/ttyACM*
/dev/ttyUSB*
```

双手分别使用两个 USB 接收器时，节点可同时打开多个串口，并根据数据帧首列判断左右手。也可以手动指定设备：

```bash
ros2 launch openarmx_hands_hig higvr_both.launch.py port:=/dev/ttyACM0
```

默认 `auto_fix_permissions:=true`。打开串口遇到权限不足时，终端可能提示输入密码以执行：

```bash
sudo chmod a+rw <串口设备>
```

这是临时处理，设备重新插拔后权限可能恢复。长期使用建议将当前用户加入 `dialout` 组并重新登录：

```bash
sudo usermod -aG dialout $USER
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
