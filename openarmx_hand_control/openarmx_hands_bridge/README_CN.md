## openarmx_hands_bridge 包简介

[English](README.md) | 中文

`openarmx_hands_bridge` 将 HIGVR 数据手套的六路动作值映射为 LinkerHand O6 控制命令。节点支持左手、右手或双手，并允许通过启动参数调整话题、发布频率和映射范围。

## 🔄 数据流

输入消息类型为 `std_msgs/msg/Int32MultiArray`：

| 输入话题 | 默认数据顺序 |
|----------|--------------|
| `/higvr/left_hand` | `[拇指, 食指, 中指, 无名指, 小指, 拇指横摆]` |
| `/higvr/right_hand` | `[拇指, 食指, 中指, 无名指, 小指, 拇指横摆]` |

输出消息类型为 `sensor_msgs/msg/JointState`：

| 输出话题 | `position` 顺序 |
|----------|-----------------|
| `/openarmx/o6/left/command` | `[拇指弯曲, 拇指横摆, 食指, 中指, 无名指, 小指]` |
| `/openarmx/o6/right/command` | `[拇指弯曲, 拇指横摆, 食指, 中指, 无名指, 小指]` |

## 📐 默认映射

HIGVR 值范围为 `0-100`，O6 命令范围为 `0-255`。HIGVR 的 `100` 接近伸直，O6 的 `255` 接近张开。

| HIGVR 输入 | O6 输出 | 默认映射 |
|------------|---------|----------|
| 拇指弯曲 | `position[0]` | `0-100` → `0-200` |
| 食指弯曲 | `position[2]` | `0-100` → `0-255` |
| 中指弯曲 | `position[3]` | `0-100` → `0-255` |
| 无名指弯曲 | `position[4]` | `0-100` → `0-255` |
| 小指弯曲 | `position[5]` | `0-100` → `0-255` |
| 拇指横摆 | `position[1]` | `0-100` → `0-255` |

因此默认拇指横摆为：

```text
HIGVR 0   -> O6 position[1] 0
HIGVR 100 -> O6 position[1] 255
```

## 🛠️ 构建

```bash
cd <openarmx_ws_hand>
colcon build --symlink-install --packages-select openarmx_hands_bridge
source install/setup.bash
```

## 🚀 启动

依次启动 HIGVR 读取、O6 和桥接：

```bash
ros2 launch openarmx_hands_hig higvr_both.launch.py
ros2 launch hands_bringup o6_bringup.launch.py sim:=true hand:=both
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=both
```

控制真机时，将 O6 启动命令替换为：

```bash
ros2 launch hands_bringup o6_bringup.launch.py \
  sim:=false hand:=both left_can:=can0 right_can:=can1
```

只桥接单手：

```bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=left
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py hand:=right
```

## ⚙️ 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `hand` | `both` | `left`、`right` 或 `both` |
| `publish_rate` | `100.0` | O6 命令发布频率 |
| `left_input_topic` | `/higvr/left_hand` | 左手 HIGVR 输入 |
| `right_input_topic` | `/higvr/right_hand` | 右手 HIGVR 输入 |
| `left_output_topic` | `/openarmx/o6/left/command` | 左手 O6 输出 |
| `right_output_topic` | `/openarmx/o6/right/command` | 右手 O6 输出 |
| `thumb_close_value` | `0.0` | 拇指最大弯曲输出值 |
| `finger_close_value` | `0.0` | 其他手指最大弯曲输出值 |
| `thumb_yaw_min` | `0.0` | 横摆输入为 `0` 时的输出值 |
| `thumb_yaw_max` | `255.0` | 横摆输入为 `100` 时的输出值 |
| `pinky_open_value` | `255.0` | 小指伸直输出值 |

例如，需要反转拇指横摆方向时可交换上下限：

```bash
ros2 launch openarmx_hands_bridge higvr_to_o6.launch.py \
  thumb_yaw_min:=255.0 thumb_yaw_max:=0.0
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
