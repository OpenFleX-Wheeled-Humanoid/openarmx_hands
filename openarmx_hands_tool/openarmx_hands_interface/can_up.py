#!/usr/bin/env python3
import argparse
import subprocess
import sys
from pathlib import Path
from typing import Iterable, List


SYS_NET = Path("/sys/class/net")


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
    try:
        return (SYS_NET / name / "operstate").read_text().strip().lower() == "up"
    except OSError:
        return False


def choose_interfaces(requested: Iterable[str] = None) -> List[str]:
    if requested:
        return list(requested)
    return [name for name in list_can_interfaces() if is_real_can_interface(name)]


def run_ip_command(args: List[str]) -> bool:
    cmd = ["sudo", "ip", *args]
    print("$ " + " ".join(cmd))
    result = subprocess.run(cmd, check=False)
    return result.returncode == 0


def enable_can_interface(name: str, bitrate: int, loopback: bool = False) -> bool:
    down_ok = run_ip_command(["link", "set", name, "down"])
    loopback_value = "on" if loopback else "off"
    up_ok = run_ip_command(
        ["link", "set", name, "up", "type", "can", "bitrate", str(bitrate), "loopback", loopback_value]
    )
    return down_ok and up_ok


def main(args=None):
    parser = argparse.ArgumentParser(description="打开 OpenArmX 手部 CAN 接口")
    parser.add_argument(
        "interfaces",
        nargs="*",
        help="CAN 接口名，例如 can0 can1。不填则自动处理所有真实 CAN 接口。",
    )
    parser.add_argument("--bitrate", type=int, default=1000000, help="CAN 波特率，默认 1000000")
    parser.add_argument("--loopback", action="store_true", help="启用 CAN loopback")
    parsed = parser.parse_args(args=args)

    interfaces = choose_interfaces(parsed.interfaces)
    if not interfaces:
        print("未检测到真实 CAN 接口。可用 ip link show 检查硬件是否识别。")
        return 1

    failed = []
    for name in interfaces:
        if not is_real_can_interface(name):
            print(f"跳过 {name}: 不是真实 CAN 接口")
            continue
        print(f"打开 {name}: bitrate={parsed.bitrate}, loopback={parsed.loopback}")
        if not enable_can_interface(name, parsed.bitrate, parsed.loopback):
            failed.append(name)
            continue
        print(f"{name}: {'UP' if interface_is_up(name) else 'DOWN'}")

    if failed:
        print("打开失败: " + ", ".join(failed))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
