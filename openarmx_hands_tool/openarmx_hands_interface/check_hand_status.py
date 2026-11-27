#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
#
# Copyright (c) 2026 Chengdu Changshu Robot Co., Ltd.
# https://www.openarmx.com
#
# This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike
# 4.0 International License (CC BY-NC-SA 4.0).
#
# To view a copy of this license, visit:
# http://creativecommons.org/licenses/by-nc-sa/4.0/
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

"""
@File    :   check_hand_status.py
@Desc    :   OpenArmX O6 手部电机状态查询
"""

import argparse
import socket
import struct
import sys
import time
import os
import unicodedata
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


SYS_NET = Path("/sys/class/net")
CAN_FRAME = struct.Struct("=IB3x8s")
DEFAULT_HANDS = (("right", 0x27), ("left", 0x28))
JOINTS = (
    ("thumb_cmc_pitch", "拇指"),
    ("thumb_cmc_yaw", "虎口"),
    ("index_mcp_pitch", "食指"),
    ("middle_mcp_pitch", "中指"),
    ("ring_mcp_pitch", "无名指"),
    ("pinky_mcp_pitch", "小指"),
)
SIDE_NAMES = {
    "right": "右手",
    "left": "左手",
}
TABLE_COLUMNS = (
    ("position", "位置"),
    ("speed", "速度"),
    ("torque", "力矩"),
    ("temperature", "温度"),
    ("fault", "故障"),
    ("current", "电流"),
)
GREEN = "\033[32m"
RED = "\033[31m"
RESET = "\033[0m"


class ChineseArgumentParser(argparse.ArgumentParser):
    def format_help(self) -> str:
        return super().format_help().replace("usage:", "用法:", 1)


def list_can_interfaces() -> List[str]:
    return sorted(path.name for path in SYS_NET.glob("can*") if path.is_dir())


def is_real_can_interface(name: str) -> bool:
    device_path = SYS_NET / name / "device"
    type_path = SYS_NET / name / "type"
    try:
        return device_path.exists() and type_path.read_text().strip() == "280"
    except OSError:
        return False


def interface_is_up(name: str) -> bool:
    flags_path = SYS_NET / name / "flags"
    try:
        flags = int(flags_path.read_text().strip(), 16)
        return bool(flags & 0x1)
    except OSError:
        return False


def choose_interfaces(requested: Optional[Iterable[str]] = None) -> List[str]:
    if requested:
        return list(requested)
    return [
        name
        for name in list_can_interfaces()
        if is_real_can_interface(name) and interface_is_up(name)
    ]


def parse_can_id(value: str) -> int:
    try:
        can_id = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"无效 CAN ID: {value}") from exc
    if can_id < 0 or can_id > 0x7FF:
        raise argparse.ArgumentTypeError("CAN ID 必须在 0x000-0x7ff 范围内")
    return can_id


def printable_serial(values: Sequence[int]) -> str:
    text = "".join(chr(value) for value in values if 32 <= value <= 126)
    return text if text else "-"


def format_values(values: Optional[Sequence[int]]) -> str:
    return "未响应" if values is None else str(list(values))


def display_width(text: str) -> int:
    return sum(2 if unicodedata.east_asian_width(char) in ("F", "W") else 1 for char in text)


def pad_right(text: str, width: int) -> str:
    return text + " " * max(0, width - display_width(text))


def pad_left(text: str, width: int) -> str:
    return " " * max(0, width - display_width(text)) + text


def use_color() -> bool:
    return sys.stdout.isatty() and os.environ.get("NO_COLOR") is None


def colored(text: str, code: str) -> str:
    if not use_color():
        return text
    return f"{code}{text}{RESET}"


def status_dot(ok: bool) -> str:
    return colored("●", GREEN if ok else RED)


class O6CanStatusClient:
    def __init__(self, interface: str, timeout: float):
        self.interface = interface
        self.timeout = timeout
        self.sock = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
        self.sock.bind((interface,))
        self.sock.settimeout(timeout)

    def close(self) -> None:
        self.sock.close()

    def drain(self) -> None:
        old_timeout = self.sock.gettimeout()
        self.sock.settimeout(0.0)
        try:
            while True:
                try:
                    self.sock.recv(CAN_FRAME.size)
                except (BlockingIOError, socket.timeout):
                    break
        finally:
            self.sock.settimeout(old_timeout)

    def send_frame(self, can_id: int, frame_type: int, payload: Sequence[int] = ()) -> None:
        data = bytes([frame_type & 0xFF, *[int(value) & 0xFF for value in payload]])[:8]
        frame = CAN_FRAME.pack(can_id, len(data), data.ljust(8, b"\x00"))
        self.sock.send(frame)

    def receive_matching(
        self,
        can_id: int,
        frame_type: int,
        timeout: Optional[float] = None,
    ) -> Optional[List[int]]:
        deadline = time.monotonic() + (self.timeout if timeout is None else timeout)
        while time.monotonic() < deadline:
            remaining = max(0.001, deadline - time.monotonic())
            self.sock.settimeout(remaining)
            try:
                raw = self.sock.recv(CAN_FRAME.size)
            except socket.timeout:
                break
            rx_id, dlc, data = CAN_FRAME.unpack(raw)
            payload = list(data[:dlc])
            if rx_id != can_id or not payload or payload[0] != frame_type:
                continue
            response = payload[1:]
            if response:
                return response
        self.sock.settimeout(self.timeout)
        return None

    def request_values(self, can_id: int, frame_type: int, timeout: Optional[float] = None) -> Optional[List[int]]:
        self.drain()
        self.send_frame(can_id, frame_type)
        return self.receive_matching(can_id, frame_type, timeout=timeout)

    def request_version(self, can_id: int) -> Optional[List[int]]:
        version = self.request_values(can_id, 0x64, timeout=max(self.timeout, 0.25))
        if version is None:
            version = self.request_values(can_id, 0xC2, timeout=max(self.timeout, 0.25))
        return version

    def request_serial_number(self, can_id: int) -> str:
        self.drain()
        self.send_frame(can_id, 0xC0)
        deadline = time.monotonic() + max(self.timeout, 0.35)
        chunks: Dict[int, List[int]] = {}
        while time.monotonic() < deadline:
            remaining = max(0.001, deadline - time.monotonic())
            self.sock.settimeout(remaining)
            try:
                raw = self.sock.recv(CAN_FRAME.size)
            except socket.timeout:
                break
            rx_id, dlc, data = CAN_FRAME.unpack(raw)
            payload = list(data[:dlc])
            if rx_id != can_id or len(payload) < 2 or payload[0] != 0xC0:
                continue
            chunk_index = payload[1]
            if 0 <= chunk_index <= 3:
                chunks[chunk_index] = payload[2:]
        self.sock.settimeout(self.timeout)
        values: List[int] = []
        for index in sorted(chunks):
            values.extend(chunks[index])
        return printable_serial(values)


def query_hand(client: O6CanStatusClient, side: str, can_id: int) -> Optional[Dict[str, object]]:
    state = client.request_values(can_id, 0x01)
    if state is None:
        return None

    return {
        "side": side,
        "can_id": can_id,
        "position": state,
        "speed": client.request_values(can_id, 0x05),
        "torque": client.request_values(can_id, 0x02),
        "temperature": client.request_values(can_id, 0x33),
        "fault": client.request_values(can_id, 0x35),
        "current": client.request_values(can_id, 0x36),
        "version": client.request_version(can_id),
        "serial_number": client.request_serial_number(can_id),
    }


def print_hand_status(interface: str, status: Dict[str, object]) -> None:
    side = status["side"]
    side_name = SIDE_NAMES.get(str(side), str(side))
    can_id = int(status["can_id"])
    print(f"\n  {status_dot(True)} 识别到  {side_name}  CAN ID: 0x{can_id:02x}")
    print(f"    CAN 通道: {interface}")
    print_status_table(status)
    print("    固件版本:")
    print(f"      {format_values(status['version'])}")
    print("    设备序列号:")
    print(f"      {status['serial_number']}")


def table_cell(values: object, index: int) -> str:
    if values is None:
        return "未响应"
    if not isinstance(values, Sequence) or isinstance(values, (str, bytes)):
        return str(values)
    if index >= len(values):
        return "-"
    return str(values[index])


def print_status_table(status: Dict[str, object]) -> None:
    name_width = 43
    col_width = 10
    header = pad_right("关节名称（内部标识）", name_width)
    header += "".join(pad_left(title, col_width) for _, title in TABLE_COLUMNS)

    print("    六关节状态（设备原始值）:")
    print(f"    {header}")
    for index, (joint_key, joint_name) in enumerate(JOINTS):
        name = f"{joint_name}（{joint_key}）"
        row = f"    {pad_right(name, name_width)}"
        for key, _ in TABLE_COLUMNS:
            row += pad_left(table_cell(status.get(key), index), col_width)
        print(row)


def print_hand_missing(side: str, can_id: int) -> None:
    side_name = SIDE_NAMES.get(side, side)
    print(f"\n  {status_dot(False)} 未识别到  {side_name}  CAN ID: 0x{can_id:02x}")


def main(args=None) -> int:
    parser = ChineseArgumentParser(
        description="检查 OpenArmX O6 手部电机状态",
        add_help=False,
    )
    parser._positionals.title = "位置参数"
    parser._optionals.title = "可选参数"
    parser.add_argument("-h", "--help", action="help", help="显示此帮助信息并退出")
    parser.add_argument(
        "interfaces",
        nargs="*",
        help="CAN 接口名，例如 can0 can1。不填则自动检查所有已启用的真实 CAN 接口。",
    )
    parser.add_argument("--right-id", type=parse_can_id, default=0x27, help="右手 CAN ID，默认 0x27")
    parser.add_argument("--left-id", type=parse_can_id, default=0x28, help="左手 CAN ID，默认 0x28")
    parser.add_argument("--timeout", type=float, default=0.2, help="单次查询超时时间，默认 0.2 秒")
    parser.add_argument(
        "--show-missing",
        action="store_true",
        help="显示每个 CAN 通道上未识别到的手部探测结果",
    )
    parsed = parser.parse_args(args=args)

    can_interfaces = choose_interfaces(parsed.interfaces)
    if not can_interfaces:
        print("未检测到已启用的真实 CAN 接口！")
        print("\n可能的原因:")
        print("  1. CAN 接口未启用，请先运行手部 can_up.py")
        print("  2. CAN 硬件未连接")
        print("  3. 驱动未加载")
        print("\n提示: 运行 'ip link show' 查看所有网络接口")
        return 1

    print(f"检测到可用的 CAN 通道: {sorted(can_interfaces)}")
    print("=" * 60)
    print("检查 O6 手部电机状态...")
    print("=" * 60)

    hands: Tuple[Tuple[str, int], ...] = (("right", parsed.right_id), ("left", parsed.left_id))
    detected = 0
    failed_interfaces: List[str] = []

    for interface in can_interfaces:
        if not is_real_can_interface(interface):
            print(f"\n跳过 {interface}: 不是真实 CAN 接口")
            continue
        if not interface_is_up(interface):
            print(f"\n跳过 {interface}: CAN 接口未启用")
            continue

        print(f"\n【CAN 通道: {interface}】")
        try:
            client = O6CanStatusClient(interface, timeout=max(parsed.timeout, 0.05))
        except OSError as exc:
            failed_interfaces.append(interface)
            print(f"打开 CAN 通道失败: {exc}")
            continue

        interface_detected = 0
        try:
            for side, can_id in hands:
                try:
                    status = query_hand(client, side, can_id)
                except OSError as exc:
                    side_name = SIDE_NAMES.get(side, side)
                    print(f"  {side_name}（CAN ID 0x{can_id:02x}）查询失败: {exc}")
                    continue
                if status is None:
                    if parsed.show_missing:
                        print_hand_missing(side, can_id)
                    continue
                print_hand_status(interface, status)
                interface_detected += 1
                detected += 1
        finally:
            client.close()

        if interface_detected == 0:
            print(f"  {status_dot(False)} 未识别到 O6 手部设备")

    print("\n" + "=" * 60)
    if failed_interfaces:
        print("打开失败的 CAN 通道: " + ", ".join(failed_interfaces))
    print(f"完成！共检测到 {detected} 个 O6 手部设备")
    return 0 if detected > 0 else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n用户中断操作 (Ctrl+C)")
        sys.exit(130)
    except Exception as exc:
        print(f"\n程序异常: {exc}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
