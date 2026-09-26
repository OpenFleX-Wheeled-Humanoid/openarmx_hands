import os
from pathlib import Path
import runpy
import unittest
import xml.etree.ElementTree as ET

import xacro
from launch import LaunchContext


WORKSPACE = Path(__file__).resolve().parents[5]
DESCRIPTION = WORKSPACE / "src/openflex_integrated/openarmx_integrated_description/urdf"


def _render(filename):
    return ET.fromstring(
        xacro.process_file(
            DESCRIPTION / filename,
            mappings={"use_fake_hardware": "false"},
        ).toxml()
    )


class IntegratedO6XacroTest(unittest.TestCase):
    def test_standard_and_o6_models_keep_their_end_effectors_isolated(self):
        standard = _render("openarmx_integrated_robot.urdf.xacro")
        o6_path = DESCRIPTION / "openarmx_integrated_robot_o6.urdf.xacro"
        self.assertTrue(o6_path.is_file())
        o6 = _render(o6_path.name)

        standard_joints = {joint.get("name") for joint in standard.iter("joint")}
        o6_joints = {joint.get("name") for joint in o6.iter("joint")}
        self.assertIn("openarmx_left_finger_joint1", standard_joints)
        self.assertIn("openarmx_right_finger_joint1", standard_joints)
        self.assertNotIn("openarmx_left_finger_joint1", o6_joints)
        self.assertNotIn("openarmx_right_finger_joint1", o6_joints)

        for side in ("left", "right"):
            control = o6.find(f"./ros2_control[@name='openarmx_{side}_hardware_interface']")
            self.assertIsNotNone(control)
            hand = control.find("./hardware/param[@name='hand']")
            self.assertIsNotNone(hand)
            self.assertEqual(hand.text.lower(), "false")

    def test_o6_launch_includes_lift_ros2_control_system(self):
        launch_path = (
            WORKSPACE
            / "src/openflex_integrated/openarmx_integrated_bringup/launch"
            / "integrated_robot_o6_bringup.launch.py"
        )
        module = runpy.run_path(launch_path)
        context = LaunchContext()
        defaults = {
            "use_fake_hardware": "true",
            "chassis_steering_can": "can5",
            "chassis_driving_can": "can4",
            "left_arm_can": "can1",
            "right_arm_can": "can0",
            "lift_can": "can3",
            "head_can": "can2",
            "head_control_mode": "csp",
            "enable_head": "true",
            "control_mode": "mit",
            "left_o6_can_id": "0x28",
            "right_o6_can_id": "0x27",
            "o6_update_rate": "50.0",
        }
        defaults.update(module["_lift_defaults"]())
        context.launch_configurations.update(defaults)

        root = ET.fromstring(module["_render_o6_description"](context))
        self.assertIsNotNone(root.find("./ros2_control[@name='LiftSlideSystem']"))


if __name__ == "__main__":
    unittest.main()
