## hands_description 简介

[English](README.md) | 中文

`hands_description` 提供 LinkerHand O6 的 URDF 资源、RViz 配置、设备值到模型关节的映射以及 ROS 2 机器人描述生成，支持独立显示左手、右手或双手模型。

该包不包含 CAN 或 RS485 通信，硬件访问由 `hands_hardware` 实现。

## 🧩 组成

| 组件 | 说明 |
|------|------|
| `o6_description_core` | O6 姿态归一化、URDF 映射和模型生成 C++ 库 |
| `o6_joint_state_mapper_node` | 将 O6 六路状态或命令转换为 URDF 关节状态 |
| `o6_robot_description_node` | 生成模型并写入节点的 `robot_description` 参数 |
| `o6_robot_description_cli` | 将生成的 O6 URDF 输出到标准输出 |
| `urdf/o6/` | 左右 O6 模型和网格资源 |
| `rviz/o6.rviz` | O6 独立显示的 RViz 配置 |

## 🛠️ 构建

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-select hands_description
source install/setup.bash
```

## 🤖 O6 姿态格式

六路输入值的顺序为：

```text
[拇指弯曲, 拇指横摆, 食指, 中指, 无名指, 小指]
```

每个值都会被限制到 `0-255`。接近 `255` 时映射到 URDF 关节的张开端，接近 `0` 时映射到弯曲端。默认张开姿态为：

```text
[200, 255, 255, 255, 255, 255]
```

映射后的最大关节角度为：拇指弯曲 `0.58 rad`、拇指横摆 `1.36 rad`，其余手指均为 `1.6 rad`。

生成的关节名称带有左右前缀，例如：

```text
left_lh_thumb_cmc_pitch
right_rh_thumb_cmc_pitch
```

## 📡 关节状态映射节点

直接运行映射节点：

```bash
ros2 run hands_description o6_joint_state_mapper_node --ros-args \
  -p hand:=both \
  -p left_input_topic:=/openarmx/o6/left/command \
  -p right_input_topic:=/openarmx/o6/right/command \
  -p joint_states_topic:=/joint_states \
  -p publish_rate:=50.0
```

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `hand` | `both` | `left`、`right` 或 `both` |
| `left_input_topic` | `/openarmx/o6/left/command` | 左手六路输入 |
| `right_input_topic` | `/openarmx/o6/right/command` | 右手六路输入 |
| `joint_states_topic` | `/joint_states` | 映射后的 URDF 关节状态输出 |
| `publish_rate` | `50.0` | 输出频率，单位 Hz |

输入和输出均使用 `sensor_msgs/msg/JointState`。节点会将选中的手初始化为张开姿态，并持续发布最新的映射状态。

## 🧱 机器人描述

通过命令行生成双手 URDF：

```bash
ros2 run hands_description o6_robot_description_cli \
  "$(ros2 pkg prefix hands_description)/share/hands_description" both
```

最后一个参数可使用 `left`、`right` 或 `both`。独立双手模型将左手放在 `x=0.20 m` 并绕 Z 轴旋转 π，将右手放在 `x=-0.20 m`。

描述节点支持以下参数：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `package_share` | 空 | 安装后的 `hands_description` share 目录 |
| `hand` | `both` | 模型选择 |

## 💻 C++ 接口

导出的 `hands_description::o6_description_core` 目标提供：

```cpp
#include "openarmx_linker_o6_c/description/joint_mapping.hpp"
#include "openarmx_linker_o6_c/description/robot_description.hpp"
```

主要函数包括 `normalize_range()`、`range_to_arc()`、`o6_urdf_joint_positions()`、`prefixed_o6_joint_names()` 和 `build_robot_description()`。

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
