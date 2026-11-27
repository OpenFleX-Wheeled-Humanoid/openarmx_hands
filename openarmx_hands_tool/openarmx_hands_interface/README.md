## openarmx_hands_interface Overview

English | [中文](README_CN.md)

`openarmx_hands_interface` provides command-line utilities for configuring physical SocketCAN interfaces and diagnosing an OpenArmX dual-arm robot equipped with two LinkerHand O6 hands.

This directory is a source-run utility collection, not an independent ROS 2 package. It has no `package.xml` or build configuration, does not participate in `colcon build`, and should be run directly with Python.

## Main Files

| File | Description |
|------|-------------|
| `can_up.py` | Bring up detected physical CAN interfaces |
| `can_down.py` | Bring down detected physical CAN interfaces |
| `check_hand_status.py` | Query the two O6 hands without starting ROS 2 drivers |
| `check_motor_status.py` | Check both arms and O6 hands in independent-CAN mode |
| `check_motor_status_shared_bus.py` | Check both arms and O6 hands in default shared-bus mode |

## Requirements

- Linux with SocketCAN support
- Python 3
- `iproute2` (`ip link`)
- Physical CAN adapters and their kernel drivers
- `openarmx_arm_driver` for the arm section of both complete-robot checkers

The CAN configuration scripts execute `sudo ip link`, so the terminal may request the system password.

## Hardware Mapping

Default shared-bus mode:

| Device | CAN interface | CAN ID |
|--------|---------------|--------|
| Right arm | `can0` | Motor IDs `1-7` |
| Right O6 hand | `can0` | `0x27` (`39`) |
| Left arm | `can1` | Motor IDs `1-7` |
| Left O6 hand | `can1` | `0x28` (`40`) |

Independent-CAN mode:

| Device | CAN interface | CAN ID |
|--------|---------------|--------|
| Right arm | `can0` | Motor IDs `1-7` |
| Left arm | `can1` | Motor IDs `1-7` |
| Right O6 hand | `can2` | `0x27` (`39`) |
| Left O6 hand | `can3` | `0x28` (`40`) |

The removed gripper motor ID `8` is not queried by the combined status checker.

## Enter the Tool Directory

```bash
cd ~/openarmx/openarmx_ws/src/openarmx_hands/src/openarmx_hands_tool
```

## CAN Interface Control

Bring up all detected physical CAN interfaces at the default bitrate of `1000000`:

```bash
python3 openarmx_hands_interface/can_up.py
```

Select interfaces, bitrate, or loopback mode:

```bash
python3 openarmx_hands_interface/can_up.py can0 can1 can2 can3
python3 openarmx_hands_interface/can_up.py can2 can3 --bitrate 1000000
python3 openarmx_hands_interface/can_up.py can2 --loopback
```

Bring down all active physical CAN interfaces or selected interfaces:

```bash
python3 openarmx_hands_interface/can_down.py
python3 openarmx_hands_interface/can_down.py can2 can3
```

The scripts identify a physical CAN interface through `/sys/class/net`: the interface must have Linux network type `280` and a hardware `device` entry. Virtual CAN interfaces are intentionally excluded.

## Check O6 Hands Only

Scan all active physical CAN interfaces for the default right and left O6 IDs:

```bash
python3 openarmx_hands_interface/check_hand_status.py
```

Restrict the scan or override IDs and timeout:

```bash
python3 openarmx_hands_interface/check_hand_status.py can2 can3 --show-missing
python3 openarmx_hands_interface/check_hand_status.py can2 \
  --right-id 0x27 --left-id 0x28 --timeout 0.3
```

For each detected hand, the script displays six O6 channels:

- Thumb (`thumb_cmc_pitch`)
- Thumb web (`thumb_cmc_yaw`)
- Index (`index_mcp_pitch`)
- Middle (`middle_mcp_pitch`)
- Ring (`ring_mcp_pitch`)
- Little finger (`pinky_mcp_pitch`)

Position, speed, torque, temperature, fault, and current are displayed as raw O6 response bytes. The script also queries firmware version and device serial number.

## Check the Complete Robot

Check both arms and both O6 hands using the default shared-bus mapping:

```bash
python3 openarmx_hands_interface/check_motor_status_shared_bus.py
```

Override the shared CAN interfaces when required:

```bash
python3 openarmx_hands_interface/check_motor_status_shared_bus.py \
  --right-can can0 --left-can can1 \
  --right-hand-id 0x27 --left-hand-id 0x28
```

Check the independent-CAN mapping:

```bash
python3 openarmx_hands_interface/check_motor_status.py
```

The arm section explicitly queries motor IDs `1-7` on each arm. It does not query motor ID `8`, enable motors, change modes, or send motion targets. The O6 section reuses the same read-only query protocol as `check_hand_status.py`.

Override interfaces, IDs, and timeouts when required:

```bash
python3 openarmx_hands_interface/check_motor_status.py \
  --right-arm-can can0 --left-arm-can can1 \
  --right-hand-can can2 --left-hand-can can3 \
  --right-hand-id 0x27 --left-hand-id 0x28 \
  --arm-timeout 1.0 --hand-timeout 0.2
```

The final summary reports arm motor responses out of `14` and O6 hand responses out of `2`. The command succeeds only when all 16 expected devices respond.

## Exit Status

| Script | Success status |
|--------|----------------|
| `can_up.py` | All requested CAN configuration commands completed |
| `can_down.py` | All requested CAN shutdown commands completed |
| `check_hand_status.py` | At least one O6 hand was detected |
| `check_motor_status.py` | All 14 arm motors and both O6 hands responded |
| `check_motor_status_shared_bus.py` | All 14 arm motors and both O6 hands responded |

`Ctrl+C` returns status `130` from all three status-checking scripts.

## Safety and Troubleshooting

- These tools are intended for real CAN hardware, not simulation.
- Run `can_up.py` before either status checker.
- The status checkers send query frames but do not send movement commands.
- Stop the normal arm and O6 driver processes before low-level diagnosis to avoid overlapping polling traffic.
- Use `ip -details link show type can` to inspect interface state and bitrate.
- If the complete checker cannot import `openarmx_arm_driver`, install the dependency used by `openarmx_motor_manager`.

## License

This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).

Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.

For details, see [LICENSE](../LICENSE) or visit: http://creativecommons.org/licenses/by-nc-sa/4.0/

## Author

- **OpenArmX Team**
- Company: Chengdu Changshu Robot Co., Ltd.
- Website: https://openarmx.com/

## Version

**Current Version**: 0.1.0
