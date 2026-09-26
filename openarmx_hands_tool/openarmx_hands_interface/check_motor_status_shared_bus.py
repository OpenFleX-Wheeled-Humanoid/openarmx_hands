#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
#
# Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.
# https://www.openarmx.com
#
# This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike
# 4.0 International License (CC BY-NC-SA 4.0).

"""检查共总线模式下双臂电机和左右 O6 灵巧手的完整硬件状态。"""

import sys
from typing import Optional, Sequence

from check_hand_status import ChineseArgumentParser, parse_can_id
from check_motor_status import (
    EXPECTED_ARM_MOTORS,
    EXPECTED_HANDS,
    check_arm_motors,
    check_can_interface,
    check_hands,
    positive_timeout,
)


def build_parser() -> ChineseArgumentParser:
    parser = ChineseArgumentParser(
        description="检查 OpenArmX 默认共总线模式下的双臂和左右 O6 灵巧手状态",
        add_help=False,
    )
    parser._positionals.title = "位置参数"
    parser._optionals.title = "可选参数"
    parser.add_argument("-h", "--help", action="help", help="显示此帮助信息并退出")
    parser.add_argument(
        "--right-can",
        default="can0",
        help="右臂与右手 O6 共用的 CAN 接口，默认 can0",
    )
    parser.add_argument(
        "--left-can",
        default="can1",
        help="左臂与左手 O6 共用的 CAN 接口，默认 can1",
    )
    parser.add_argument(
        "--right-hand-id",
        type=parse_can_id,
        default=0x27,
        help="右手 O6 CAN ID，默认 0x27",
    )
    parser.add_argument(
        "--left-hand-id",
        type=parse_can_id,
        default=0x28,
        help="左手 O6 CAN ID，默认 0x28",
    )
    parser.add_argument(
        "--arm-timeout",
        type=positive_timeout,
        default=1.0,
        help="机械臂单电机查询超时，默认 1.0 秒",
    )
    parser.add_argument(
        "--hand-timeout",
        type=positive_timeout,
        default=0.2,
        help="O6 单次查询超时，默认 0.2 秒",
    )
    return parser


def main(args: Optional[Sequence[str]] = None) -> int:
    parsed = build_parser().parse_args(args=args)

    print("=" * 118)
    print("OpenArmX 双臂 + O6 灵巧手状态检查（默认共总线模式）")
    print("=" * 118)
    print("硬件映射:")
    print(
        f"  右侧: {parsed.right_can} -> 机械臂 ID 1-7 + 右手 O6 0x{parsed.right_hand_id:02x}"
    )
    print(
        f"  左侧: {parsed.left_can} -> 机械臂 ID 1-7 + 左手 O6 0x{parsed.left_hand_id:02x}"
    )
    print("CAN 接口检查:")

    right_ready = check_can_interface(parsed.right_can, "右臂 + 右手 O6")
    left_ready = check_can_interface(parsed.left_can, "左臂 + 左手 O6")
    interfaces_ready = {
        "right_arm": right_ready,
        "left_arm": left_ready,
        "right_hand": right_ready,
        "left_hand": left_ready,
    }

    arm_detected = check_arm_motors(
        parsed.right_can,
        parsed.left_can,
        parsed.arm_timeout,
        interfaces_ready,
    )
    hand_detected = check_hands(
        parsed.right_can,
        parsed.left_can,
        parsed.right_hand_id,
        parsed.left_hand_id,
        parsed.hand_timeout,
        interfaces_ready,
    )

    print("\n" + "=" * 118)
    print("检查结果汇总（默认共总线模式）")
    print("=" * 118)
    print(
        f"  机械臂电机: {arm_detected}/{EXPECTED_ARM_MOTORS} "
        "响应（左右臂各 7 个）"
    )
    print(f"  O6 灵巧手: {hand_detected}/{EXPECTED_HANDS} 响应")
    complete = arm_detected == EXPECTED_ARM_MOTORS and hand_detected == EXPECTED_HANDS
    print(
        "  结果: "
        + ("全部设备正常响应" if complete else "存在未响应或检查失败的设备")
    )
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
