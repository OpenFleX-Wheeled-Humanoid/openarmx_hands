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


def choose_interfaces(requested: Iterable[str] = None, only_up: bool = False) -> List[str]:
    if requested:
        interfaces = list(requested)
    else:
        interfaces = [name for name in list_can_interfaces() if is_real_can_interface(name)]
    if only_up:
        interfaces = [name for name in interfaces if interface_is_up(name)]
    return interfaces


def run_ip_command(args: List[str]) -> bool:
    cmd = ["sudo", "ip", *args]
    print("$ " + " ".join(cmd))
    result = subprocess.run(cmd, check=False)
    return result.returncode == 0


def disable_can_interface(name: str) -> bool:
    return run_ip_command(["link", "set", name, "down"])


def main(args=None):
    parser = argparse.ArgumentParser(description="关闭 OpenArmX 手部 CAN 接口")
    parser.add_argument(
        "interfaces",
        nargs="*",
        help="CAN 接口名，例如 can0 can1。不填则自动处理所有真实 CAN 接口。",
    )
    parsed = parser.parse_args(args=args)

    interfaces = choose_interfaces(parsed.interfaces, only_up=not parsed.interfaces)
    if not interfaces:
        print("未检测到需要关闭的真实 CAN 接口。")
        return 0

    failed = []
    for name in interfaces:
        if not is_real_can_interface(name):
            print(f"跳过 {name}: 不是真实 CAN 接口")
            continue
        print(f"关闭 {name}")
        if not disable_can_interface(name):
            failed.append(name)
            continue
        print(f"{name}: {'UP' if interface_is_up(name) else 'DOWN'}")

    if failed:
        print("关闭失败: " + ", ".join(failed))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
