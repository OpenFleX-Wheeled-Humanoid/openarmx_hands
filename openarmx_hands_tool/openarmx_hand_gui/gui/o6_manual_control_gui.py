import json
import threading
import tkinter as tk
from tkinter import ttk

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from std_msgs.msg import String


O6_RANGE_NAMES = [
    "thumb_cmc_pitch",
    "thumb_cmc_yaw",
    "index_mcp_pitch",
    "middle_mcp_pitch",
    "ring_mcp_pitch",
    "pinky_mcp_pitch",
]
O6_OPEN_POSE = [200, 255, 255, 255, 255, 255]


def normalize_range(values):
    pose = [float(v) for v in values]
    if len(pose) < 6:
        pose.extend(O6_OPEN_POSE[len(pose):])
    return [max(0.0, min(255.0, v)) for v in pose[:6]]


class O6ManualControlNode(Node):
    def __init__(self):
        super().__init__("openarmx_o6_manual_control_gui")
        self.command_publishers = {}
        self.setting_publishers = {}

    def command_pub(self, side: str):
        if side not in self.command_publishers:
            self.command_publishers[side] = self.create_publisher(
                JointState, f"/openarmx/o6/{side}/command", 10
            )
        return self.command_publishers[side]

    def setting_pub(self, side: str):
        if side not in self.setting_publishers:
            self.setting_publishers[side] = self.create_publisher(
                String, f"/openarmx/o6/{side}/setting_cmd", 10
            )
        return self.setting_publishers[side]

    def publish_pose(self, side: str, pose):
        msg = JointState()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.name = O6_RANGE_NAMES
        msg.position = [float(v) for v in normalize_range(pose)]
        msg.velocity = [0.0] * 6
        msg.effort = [0.0] * 6
        self.command_pub(side).publish(msg)

    def publish_setting(self, side: str, command: str, key: str, values):
        msg = String()
        msg.data = json.dumps({"setting_cmd": command, "params": {key: [int(v) for v in values]}})
        self.setting_pub(side).publish(msg)


class O6ManualControlGui:
    def __init__(self, node: O6ManualControlNode):
        self.node = node
        self.root = tk.Tk()
        self.root.title("OpenArmX O6 手动控制")
        self.root.geometry("760x520")
        self.root.minsize(680, 460)

        self.side_var = tk.StringVar(value="both")
        self.pose_vars = [tk.IntVar(value=int(v)) for v in O6_OPEN_POSE]
        self.speed_var = tk.IntVar(value=255)
        self.torque_var = tk.IntVar(value=255)
        self.auto_publish = tk.BooleanVar(value=True)
        self.status_var = tk.StringVar(value="就绪")

        self._build()
        self._schedule_publish()

    def _build(self):
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(1, weight=1)

        top = ttk.Frame(self.root, padding=10)
        top.grid(row=0, column=0, sticky="ew")
        top.columnconfigure(5, weight=1)

        ttk.Label(top, text="控制对象").grid(row=0, column=0, padx=(0, 6))
        ttk.Combobox(
            top,
            textvariable=self.side_var,
            values=("both", "left", "right"),
            width=10,
            state="readonly",
        ).grid(row=0, column=1, padx=(0, 16))
        ttk.Checkbutton(top, text="连续发布", variable=self.auto_publish).grid(row=0, column=2, padx=(0, 16))
        ttk.Button(top, text="张开", command=self.set_open_pose).grid(row=0, column=3, padx=4)
        ttk.Button(top, text="握拳", command=self.set_closed_pose).grid(row=0, column=4, padx=4)
        ttk.Button(top, text="立即发布", command=self.publish_pose).grid(row=0, column=5, sticky="w", padx=4)

        sliders = ttk.LabelFrame(self.root, text="O6 位置 0-255", padding=10)
        sliders.grid(row=1, column=0, sticky="nsew", padx=10, pady=(0, 10))
        sliders.columnconfigure(1, weight=1)
        for row, (name, var) in enumerate(zip(O6_RANGE_NAMES, self.pose_vars)):
            ttk.Label(sliders, text=name, width=18).grid(row=row, column=0, sticky="w", pady=5)
            ttk.Scale(sliders, from_=0, to=255, variable=var, orient="horizontal").grid(
                row=row, column=1, sticky="ew", padx=8
            )
            ttk.Spinbox(sliders, from_=0, to=255, textvariable=var, width=5, command=self.publish_pose).grid(
                row=row, column=2, sticky="e"
            )

        settings = ttk.LabelFrame(self.root, text="参数", padding=10)
        settings.grid(row=2, column=0, sticky="ew", padx=10, pady=(0, 10))
        settings.columnconfigure(1, weight=1)

        ttk.Label(settings, text="速度").grid(row=0, column=0, sticky="w")
        ttk.Scale(settings, from_=0, to=255, variable=self.speed_var, orient="horizontal").grid(
            row=0, column=1, sticky="ew", padx=8
        )
        ttk.Spinbox(settings, from_=0, to=255, textvariable=self.speed_var, width=5).grid(row=0, column=2)
        ttk.Button(settings, text="应用速度", command=self.publish_speed).grid(row=0, column=3, padx=10)

        ttk.Label(settings, text="最大力矩").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Scale(settings, from_=0, to=255, variable=self.torque_var, orient="horizontal").grid(
            row=1, column=1, sticky="ew", padx=8, pady=(8, 0)
        )
        ttk.Spinbox(settings, from_=0, to=255, textvariable=self.torque_var, width=5).grid(row=1, column=2, pady=(8, 0))
        ttk.Button(settings, text="应用力矩", command=self.publish_torque).grid(row=1, column=3, padx=10, pady=(8, 0))

        ttk.Label(self.root, textvariable=self.status_var, padding=(10, 0, 10, 10)).grid(row=3, column=0, sticky="ew")

    def sides(self):
        side = self.side_var.get()
        return ["left", "right"] if side == "both" else [side]

    def pose(self):
        return [var.get() for var in self.pose_vars]

    def set_open_pose(self):
        for var, value in zip(self.pose_vars, O6_OPEN_POSE):
            var.set(int(value))
        self.publish_pose()

    def set_closed_pose(self):
        for var in self.pose_vars:
            var.set(0)
        self.publish_pose()

    def publish_pose(self):
        pose = self.pose()
        for side in self.sides():
            self.node.publish_pose(side, pose)
        self.status_var.set(f"已发布位置: {self.side_var.get()} {pose}")

    def publish_speed(self):
        values = [self.speed_var.get()] * 6
        for side in self.sides():
            self.node.publish_setting(side, "set_speed", "speed", values)
        self.status_var.set(f"已发布速度: {self.side_var.get()} {values}")

    def publish_torque(self):
        values = [self.torque_var.get()] * 6
        for side in self.sides():
            self.node.publish_setting(side, "set_torque", "torque", values)
        self.status_var.set(f"已发布最大力矩: {self.side_var.get()} {values}")

    def _schedule_publish(self):
        if self.auto_publish.get():
            self.publish_pose()
        self.root.after(100, self._schedule_publish)

    def run(self):
        self.root.protocol("WM_DELETE_WINDOW", self.root.quit)
        self.root.mainloop()


def main(args=None):
    rclpy.init(args=args)
    node = O6ManualControlNode()
    spin_thread = threading.Thread(target=rclpy.spin, args=(node,), daemon=True)
    spin_thread.start()
    try:
        O6ManualControlGui(node).run()
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
