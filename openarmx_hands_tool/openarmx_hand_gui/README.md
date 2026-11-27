# openarmx_hand_gui User Guide

English | [中文](README_CN.md)

## Build

```bash
cd /home/openarmx/openarmx_hands
colcon build --symlink-install --packages-select openarmx_hand_gui
source install/setup.bash
```

## O6 Manual Control GUI

Start the O6 simulation or real-robot bringup first, then launch the GUI:

```bash
ros2 run openarmx_hand_gui o6_manual_control_gui
```

The GUI publishes to:

```text
/openarmx/o6/left/command
/openarmx/o6/right/command
/openarmx/o6/left/setting_cmd
/openarmx/o6/right/setting_cmd
```

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.

For details, see [LICENSE](LICENSE) or visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

## Author

- **OpenArmX Team**
- Company: Chengdu Changshu Robot Co., Ltd.
- Website: https://openarmx.com/

## Version

**Current Version**: 0.0.0

## Acknowledgements

This package is part of the OpenArmX robotics ecosystem, developed for research and industrial applications in collaborative robotics.

---

## 📞 Contact Us

### Chengdu Changshu Robot Co., Ltd.

| Contact | Details |
|---------|---------|
| 📧 Email | openarmrobot@gmail.com |
| 📱 Phone/WeChat | +86-17746530375 |
| 🌐 Website | <https://openarmx.com/> |
| 📍 Address | Huacheng Machinery Factory, No.11 Xinye 8th Street, West Area, TEDA |
| 👤 Contact Person | Mr. Wang |
