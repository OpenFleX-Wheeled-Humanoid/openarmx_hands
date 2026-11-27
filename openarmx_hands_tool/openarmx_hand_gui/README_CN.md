# openarmx_hand_gui 使用说明

[English](README.md) | 中文

## 构建

```bash
cd /home/openarmx/openarmx_hands
colcon build --symlink-install --packages-select openarmx_hand_gui
source install/setup.bash
```

## O6 手动控制 GUI

先启动 O6 仿真或真机 bringup，再启动 GUI：

```bash
ros2 run openarmx_hand_gui o6_manual_control_gui
```

GUI 发布：

```text
/openarmx/o6/left/command
/openarmx/o6/right/command
/openarmx/o6/left/setting_cmd
/openarmx/o6/right/setting_cmd
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

**Chengdu Changshu Robot Co., Ltd.**

| 联系方式 | 信息 |
|----------|------|
| 📧 邮箱 | openarmrobot@gmail.com |
| 📱 电话/微信 | +86-17746530375 |
| 🌐 官网 | <https://openarmx.com/> |
| 📍 地址 | 天津经济技术开发区西区新业八街11号华诚机械厂 |
| 👤 联系人 | 王先生 |
