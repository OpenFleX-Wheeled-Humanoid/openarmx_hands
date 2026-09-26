## openarmx_hand_description 简介

[English](README.md) | 中文

`openarmx_hand_description` 是用于组合 OpenArmX 双 V10 机械臂与两只 LinkerHand O6 的 ROS 2 Python 模型包。它在运行时生成完整 URDF，并保持上游 `openarmx_description` 和 `hands_description` 包不变。

该包是供 `openarmx_hand_bringup` 调用的 Python 库，不提供独立的 ROS 2 节点或启动文件。

## 🧩 主要文件

| 路径 | 说明 |
|------|------|
| `openarmx_hand_description/model.py` | 加载、修改并组合双臂和 O6 模型 |
| `config/model.yaml` | Link 7、TCP 和 O6 安装配置 |
| `meshes/arm/v10/visual/` | 机械臂 Link 7 的替换视觉模型 |
| `meshes/arm/v10/collision/` | 机械臂 Link 7 的替换碰撞模型 |

## 🏗️ 模型组合流程

`build_robot_description()` 按以下步骤生成模型：

1. 渲染上游 `openarmx_description/urdf/robot/v10.urdf.xacro`，生成启用 ROS 2 control、未安装上游末端执行器的双 V10 模型。
2. 替换两条机械臂 `link7` 的视觉模型、碰撞模型和惯性质量。
3. 更新左右 `link7_pico_joint` 的 TCP 变换。
4. `shared_bus` 模式将左右 ros2_control 系统替换为机械臂与 O6 共总线的专用硬件插件，并加入每只 O6 的六个主动关节；`independent` 模式保留原机械臂硬件和七个关节。
5. 从 `hands_description` 加载并添加带左右前缀的 O6 模型。
6. 添加固定关节，将两只 O6 的基座连接到机械臂末端。
7. 设置命名空间时，将 `node_namespace` 注入专用硬件插件。

函数最终返回完整的 URDF XML 字符串，同时供 `ros2_control_node` 和 `robot_state_publisher` 使用。

## ⚙️ 模型配置

`config/model.yaml` 的主要默认值如下：

| 配置 | 左侧 | 右侧 |
|------|------|------|
| Link 7 质量 | `0.65 kg` | `0.65 kg` |
| O6 父链接 | `openarmx_left_link7` | `openarmx_right_link7` |
| O6 子链接 | `left_lh_hand_base_link` | `right_rh_hand_base_link` |
| 安装位置 | `0 0 0.05` | `0 0 0.05` |
| 安装旋转 | `0 0 1.57079632679` | `0 0 -1.57079632679` |

配置文件还定义了 Link 7 替换模型以及左右 Pico TCP 变换。只有实体安装结构或末端链接模型发生变化时，才需要调整这些数值。

## 🛠️ 构建

在包含本仓库的 ROS 2 工作区中执行：

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to openarmx_hand_description
source install/setup.bash
```

## 🐍 Python 接口

```python
from openarmx_hand_description import build_robot_description

robot_description = build_robot_description(
    o6_hardware_mode="shared_bus",
    use_fake_hardware="false",
    left_can_interface="can1",
    right_can_interface="can0",
    left_o6_can_id="0x28",
    right_o6_can_id="0x27",
    o6_update_rate="50.0",
    can_fd="false",
    control_mode="mit",
    node_namespace="",
)
```

支持的关键字参数会传递给上游 V10 Xacro，常用参数如下：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `o6_hardware_mode` | `shared_bus` | `shared_bus` 或 `independent` |
| `use_fake_hardware` | `false` | 是否使用机械臂仿真硬件 |
| `left_can_interface` | `can1` | 左臂 CAN 接口 |
| `right_can_interface` | `can0` | 右臂 CAN 接口 |
| `left_o6_can_id` | `0x28` | 左手 O6 CAN ID |
| `right_o6_can_id` | `0x27` | 右手 O6 CAN ID |
| `o6_update_rate` | `50.0` | O6 异步通信频率 |
| `can_fd` | `false` | 是否启用 CAN FD |
| `control_mode` | `mit` | 机械臂控制模式 |
| `node_namespace` | 空 | 注入专用硬件插件的命名空间 |

组合过程始终请求 `bimanual=true`、`ee_type=none`、`hand=false` 和 `ros2_control=true` 的 V10 模型，然后由本包添加 O6。两种模式的几何模型完全相同，只有 ros2_control 硬件边界不同。

## 🔗 启动包集成

`openarmx_hand_bringup` 会为主机器人模型调用该接口；启用重力补偿时还会再次调用。因此机械臂控制、可视化和重力计算使用完全相同的几何结构与硬件参数。

通过 bringup 包启动完整机器人：

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py
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

**当前版本**：0.1.0

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
