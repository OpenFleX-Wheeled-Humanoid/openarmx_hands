# openarmx_hand_hardware

这是 OpenArmX V10 机械臂与 LinkerHand O6 共用一条经典 CAN 总线时使用的
专用 ros2_control 硬件包。每个插件实例导出 7 个机械臂关节和 6 个 O6 主动
关节。插件内部组合现有 OpenArmX 机械臂硬件实现，不修改普通夹爪方案。

O6 通信运行在后台线程中，避免协议等待阻塞 controller manager 控制循环。
`o6_command_adapter` 保留原有 `/openarmx/o6/<side>/command` 和 `/state`
JointState 话题。
