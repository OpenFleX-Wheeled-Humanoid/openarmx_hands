#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
#
# Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.
# https://www.openarmx.com
#
# This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike
# 4.0 International License (CC BY-NC-SA 4.0).

"""检查双臂 1-7 号电机和左右 O6 灵巧手的完整硬件状态。"""

import argparse
import sys
import time
from typing import Dict, Optional, Sequence, Tuple

from check_hand_status import (
    ChineseArgumentParser,
    O6CanStatusClient,
    SIDE_NAMES,
    interface_is_up,
    is_real_can_interface,
    parse_can_id,
    print_hand_missing,
    print_hand_status,
    query_hand,
)


ARM_MOTOR_IDS = tuple(range(1, 8))
EXPECTED_ARM_MOTORS = len(ARM_MOTOR_IDS) * 2
EXPECTED_HANDS = 2


def positive_timeout(value: str) -> float:
    try:
        timeout = float(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"无效超时时间: {value}") from exc
    if timeout <= 0:
        raise argparse.ArgumentTypeError("超时时间必须大于 0")
    return timeout


def check_can_interface(interface: str, device_name: str) -> bool:
    if not is_real_can_interface(interface):
        print(f"  ✗ {device_name}: {interface} 不存在或不是真实 CAN 接口")
        return False
    if not interface_is_up(interface):
        print(f"  ✗ {device_name}: {interface} 尚未启用")
        return False
    print(f"  ✓ {device_name}: {interface} 已启用")
    return True


def print_arm_motor_status(arm_name: str, motor_id: int, info: Dict[str, object]) -> None:
    angle = float(info.get("angle", 0.0))
    velocity = float(info.get("velocity", 0.0))
    torque = float(info.get("torque", 0.0))
    temperature = float(info.get("temperature", 0.0))
    mode_status = str(info.get("mode_status", "未知"))
    fault_status = str(info.get("fault_status", "未知"))
    print(
        f"  {arm_name} | {motor_id:2d} | {angle:9.3f} | {velocity:11.3f} | "
        f"{torque:8.3f} | {temperature:6.1f}°C | {mode_status:16s} | {fault_status}"
    )


def check_arm_motors(
    right_can: str,
    left_can: str,
    timeout: float,
    interfaces_ready: Dict[str, bool],
) -> int:
    print("\n" + "=" * 118)
    print("机械臂电机状态（每条机械臂只检查 ID 1-7，不查询 ID 8）")
    print("=" * 118)

    if not interfaces_ready["right_arm"] or not interfaces_ready["left_arm"]:
        print("机械臂 CAN 接口不完整，跳过双臂驱动初始化。")
        return 0

    try:
        from openarmx_arm_driver import Robot
    except ImportError as exc:
        print(f"无法导入 openarmx_arm_driver，跳过机械臂检查: {exc}")
        return 0

    robot = None
    detected = 0
    try:
        robot = Robot(
            right_can_channel=right_can,
            left_can_channel=left_can,
            motor_ids=list(ARM_MOTOR_IDS),
            auto_enable_can=False,
        )
        print("  臂   | ID | 角度(rad) | 速度(rad/s) | 力矩(Nm) |  温度    | 模式             | 状态")
        print("-" * 118)
        arms = (
            ("右臂", robot.right_arm),
            ("左臂", robot.left_arm),
        )
        for arm_index, (arm_name, arm) in enumerate(arms):
            if arm_index:
                print("-" * 118)
            for motor_id in ARM_MOTOR_IDS:
                try:
                    info = arm.get_status(motor_id, timeout=timeout)
                except Exception as exc:
                    print(f"  {arm_name} | {motor_id:2d} | 查询异常: {exc}")
                    continue
                if info is None:
                    print(f"  {arm_name} | {motor_id:2d} | 无响应")
                else:
                    print_arm_motor_status(arm_name, motor_id, info)
                    detected += 1
                time.sleep(0.01)
    except Exception as exc:
        print(f"机械臂驱动初始化或查询失败: {exc}")
    finally:
        if robot is not None:
            robot.shutdown()
    return detected


def check_one_hand(
    side: str,
    interface: str,
    can_id: int,
    timeout: float,
    interface_ready: bool,
) -> bool:
    side_name = SIDE_NAMES[side]
    if not interface_ready:
        print(f"\n  ✗ {side_name}: CAN 接口不可用，跳过检查")
        return False

    client: Optional[O6CanStatusClient] = None
    try:
        client = O6CanStatusClient(interface, timeout=max(timeout, 0.05))
        status = query_hand(client, side, can_id)
    except OSError as exc:
        print(f"\n  ✗ {side_name}（{interface} / 0x{can_id:02x}）查询失败: {exc}")
        return False
    finally:
        if client is not None:
            client.close()

    if status is None:
        print_hand_missing(side, can_id)
        print(f"    CAN 通道: {interface}")
        return False
    print_hand_status(interface, status)
    return True


def check_hands(
    right_can: str,
    left_can: str,
    right_id: int,
    left_id: int,
    timeout: float,
    interfaces_ready: Dict[str, bool],
) -> int:
    print("\n" + "=" * 118)
    print("O6 灵巧手状态")
    print("=" * 118)
    detected = 0
    if check_one_hand("right", right_can, right_id, timeout, interfaces_ready["right_hand"]):
        detected += 1
    if check_one_hand("left", left_can, left_id, timeout, interfaces_ready["left_hand"]):
        detected += 1
    return detected


def build_parser() -> ChineseArgumentParser:
    parser = ChineseArgumentParser(
        description="检查 OpenArmX 双臂 1-7 号电机和左右 O6 灵巧手状态",
        add_help=False,
    )
    parser._positionals.title = "位置参数"
    parser._optionals.title = "可选参数"
    parser.add_argument("-h", "--help", action="help", help="显示此帮助信息并退出")
    parser.add_argument("--right-arm-can", default="can0", help="右臂 CAN 接口，默认 can0")
    parser.add_argument("--left-arm-can", default="can1", help="左臂 CAN 接口，默认 can1")
    parser.add_argument("--right-hand-can", default="can2", help="右手 O6 CAN 接口，默认 can2")
    parser.add_argument("--left-hand-can", default="can3", help="左手 O6 CAN 接口，默认 can3")
    parser.add_argument(
        "--right-hand-id", type=parse_can_id, default=0x27, help="右手 O6 CAN ID，默认 0x27"
    )
    parser.add_argument(
        "--left-hand-id", type=parse_can_id, default=0x28, help="左手 O6 CAN ID，默认 0x28"
    )
    parser.add_argument(
        "--arm-timeout", type=positive_timeout, default=1.0, help="机械臂单电机查询超时，默认 1.0 秒"
    )
    parser.add_argument(
        "--hand-timeout", type=positive_timeout, default=0.2, help="O6 单次查询超时，默认 0.2 秒"
    )
    return parser


def main(args: Optional[Sequence[str]] = None) -> int:
    parsed = build_parser().parse_args(args=args)
    assignments: Tuple[Tuple[str, str, str], ...] = (
        ("right_arm", "右臂", parsed.right_arm_can),
        ("left_arm", "左臂", parsed.left_arm_can),
        ("right_hand", "右手 O6", parsed.right_hand_can),
        ("left_hand", "左手 O6", parsed.left_hand_can),
    )

    print("=" * 118)
    print("OpenArmX 双臂 + O6 灵巧手完整状态检查")
    print("=" * 118)
    print("CAN 接口检查:")
    interfaces_ready = {
        key: check_can_interface(interface, name) for key, name, interface in assignments
    }

    arm_detected = check_arm_motors(
        parsed.right_arm_can,
        parsed.left_arm_can,
        parsed.arm_timeout,
        interfaces_ready,
    )
    hand_detected = check_hands(
        parsed.right_hand_can,
        parsed.left_hand_can,
        parsed.right_hand_id,
        parsed.left_hand_id,
        parsed.hand_timeout,
        interfaces_ready,
    )

    print("\n" + "=" * 118)
    print("检查结果汇总")
    print("=" * 118)
    print(f"  机械臂电机: {arm_detected}/{EXPECTED_ARM_MOTORS} 响应（左右臂各 7 个）")
    print(f"  O6 灵巧手: {hand_detected}/{EXPECTED_HANDS} 响应")
    complete = arm_detected == EXPECTED_ARM_MOTORS and hand_detected == EXPECTED_HANDS
    print("  结果: " + ("全部设备正常响应" if complete else "存在未响应或检查失败的设备"))
    return 0 if complete else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n用户中断操作（Ctrl+C）")
        sys.exit(130)
    except Exception as exc:
        print(f"\n程序异常: {exc}")
        import traceback

        traceback.print_exc()
        sys.exit(1)
