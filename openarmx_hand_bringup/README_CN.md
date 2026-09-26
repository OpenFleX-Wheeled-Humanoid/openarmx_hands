## openarmx_hand_bringup 简介

[English](README.md) | 中文

`openarmx_hand_bringup` 是 OpenArmX 双 V10 机械臂与两只 LinkerHand O6 的双模式 ROS 2 启动包。它既支持机械臂与 O6 共用 CAN 的专用 ros2_control hardware，也支持 O6 独立使用 `can2/can3` 的驱动方案。

该包通过 `openarmx_hand_description` 组合机器人模型，不需要修改上游的 `openarmx_description` 或 `openarmx_bringup` 包。

## 🧩 主要文件

| 路径 | 说明 |
|------|------|
| `launch/openarmx.bimanual.o6.launch.py` | 仿真与真机的统一启动入口 |
| `config/controllers.yaml` | 双臂、O6 和力矩控制器定义 |
| `config/o6.yaml` | 独立 O6 驱动的查询和发布参数 |
| `rviz/bimanual_o6.rviz` | 组合机器人的 RViz 配置 |

## 🛠️ 构建

在包含本仓库的 ROS 2 工作区中执行：

```bash
cd <openarmx_ws_new>
colcon build --symlink-install --packages-up-to openarmx_hand_bringup
source install/setup.bash
```

## 🚀 启动

### 共总线真机（默认）

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py
```

默认 CAN 分配如下：

| 设备 | 接口 | CAN ID |
|------|------|--------|
| 右臂 | `can0` | 由机械臂硬件配置定义 |
| 左臂 | `can1` | 由机械臂硬件配置定义 |
| 右手 O6 | 与右臂共用 `can0` | `39` (`0x27`) |
| 左手 O6 | 与左臂共用 `can1` | `40` (`0x28`) |

该命令等价于添加 `o6_hardware_mode:=shared_bus`。启动前只需启用 `can0` 和 `can1`，两条总线均使用经典 CAN 1 Mbps。共总线模式不支持 `can_fd=true`。

### 独立 O6 真机

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=independent
```

默认分配为右臂 `can0`、左臂 `can1`、右手 `can2/0x27`、左手 `can3/0x28`。该模式使用独立 O6 driver、joint-state mapper 和状态合并节点。

### 全仿真

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  use_fake_hardware:=true
```

`shared_bus` 模式下双臂和 O6 必须一起切换真假硬件。`independent` 模式允许用 `use_fake_o6_hardware` 单独覆盖 O6：

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  o6_hardware_mode:=independent \
  use_fake_hardware:=true use_fake_o6_hardware:=false
```

### 机器人命名空间

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  robot_namespace:=robot1
```

命名空间会应用于节点、控制器和相对话题，可用于隔离多个机器人实例。

## 🎛️ 双臂控制器

默认使用 `joint_trajectory_controller`。需要直接转发位置命令时使用：

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  robot_controller:=forward_position_controller
```

两种控制器都会控制每条机械臂的七个关节。左右 O6 分别由独立的位置控制器管理六个主动关节，controller manager 的更新频率为 `100 Hz`。

## ⚖️ 重力补偿

重力补偿默认关闭。在机械臂 MIT 模式下使用以下命令启用：

```bash
ros2 launch openarmx_hand_bringup openarmx.bimanual.o6.launch.py \
  control_mode:=mit enable_forward_effort:=true
```

启动后约一秒会加载左右臂 forward-effort controller，约两秒后启动 `openarmx_gravity_comp/gravity_comp_node`。启动文件默认对左右臂启用补偿，并设置 `g_scale=1.05`。如果在 `control_mode:=csp` 下启用 effort 转发，启动文件会直接报错。

## ⚙️ 启动参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `o6_hardware_mode` | `shared_bus` | `shared_bus` 或 `independent` |
| `use_fake_hardware` | `false` | 使用机械臂仿真硬件；共总线模式同时作用于 O6 |
| `use_fake_o6_hardware` | 空 | 独立模式的 O6 仿真覆盖值 |
| `robot_namespace` | 空 | 节点、控制器和话题的命名空间 |
| `left_can_interface` | `can1` | 左臂 CAN 接口 |
| `right_can_interface` | `can0` | 右臂 CAN 接口 |
| `left_o6_can_interface` | `can3` | 独立模式左手 CAN 接口 |
| `right_o6_can_interface` | `can2` | 独立模式右手 CAN 接口 |
| `left_o6_can_id` | `0x28` | 左手 O6 CAN ID |
| `right_o6_can_id` | `0x27` | 右手 O6 CAN ID |
| `o6_update_rate` | `50.0` | O6 异步通信频率 |
| `can_fd` | `false` | 共总线模式必须保持为 `false` |
| `control_mode` | `mit` | 机械臂硬件模式：`mit` 或 `csp` |
| `robot_controller` | `joint_trajectory_controller` | 双臂位置控制器类型 |
| `enable_forward_effort` | `false` | 启动力矩控制器和重力补偿 |
| `launch_rviz` | `true` | 是否启动 RViz |

## 📡 状态与显示流程

`shared_bus` 模式下，双臂和两只 O6 的主动关节状态由同一个 `joint_state_broadcaster` 发布到 `joint_states`。`independent` 模式下，机械臂状态与 O6 mapper 状态经过 `joint_state_merger` 合并后驱动机器人显示。

两种模式都保留 `/openarmx/o6/left/command`、`/right/command` 和对应的 `/state` 接口，因此现有 HIGVR/VR 上层控制方式不需要改变。

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
