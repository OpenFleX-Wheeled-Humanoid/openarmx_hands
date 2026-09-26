import xml.etree.ElementTree as ET
import unittest
from unittest.mock import patch

from openarmx_hand_description import model


def _base_robot_xml():
    return """<robot name=\"integrated\">
  <link name=\"openarmx_left_link7\"><visual><geometry><mesh filename=\"old.dae\" /></geometry></visual><collision><geometry><mesh filename=\"old.stl\" /></geometry></collision><inertial><mass value=\"1.0\" /></inertial></link>
  <link name=\"openarmx_right_link7\"><visual><geometry><mesh filename=\"old.dae\" /></geometry></visual><collision><geometry><mesh filename=\"old.stl\" /></geometry></collision><inertial><mass value=\"1.0\" /></inertial></link>
  <link name=\"openarmx_left_link7_pico\" />
  <link name=\"openarmx_right_link7_pico\" />
  <joint name=\"openarmx_left_link7_pico_joint\" type=\"fixed\"><origin xyz=\"0 0 0\" rpy=\"0 0 0\" /></joint>
  <joint name=\"openarmx_right_link7_pico_joint\" type=\"fixed\"><origin xyz=\"0 0 0\" rpy=\"0 0 0\" /></joint>
  <ros2_control name=\"openarmx_left_hardware_interface\"><hardware><plugin>openarmx_hardware/OpenArmX_v10HW</plugin></hardware><joint name=\"openarmx_left_joint1\" /></ros2_control>
  <ros2_control name=\"openarmx_right_hardware_interface\"><hardware><plugin>openarmx_hardware/OpenArmX_v10HW</plugin></hardware><joint name=\"openarmx_right_joint1\" /></ros2_control>
</robot>"""


class O6ModelTest(unittest.TestCase):
    def test_inject_o6_hands_replaces_arm_plugins_and_adds_twelve_control_joints(self):
        config = {
            "link7": {"mass": 0.65, "visual_mesh": "o6.dae", "collision_mesh": "o6.stl"},
            "tcp": {
                "left": {"xyz": "0 0 0", "rpy": "0 0 0"},
                "right": {"xyz": "0 0 0", "rpy": "0 0 0"},
            },
            "mount": {
                "left": {"xyz": "0 0 0", "rpy": "0 0 0", "parent": "openarmx_left_link7", "child": "left_lh_hand_base_link"},
                "right": {"xyz": "0 0 0", "rpy": "0 0 0", "parent": "openarmx_right_link7", "child": "right_rh_hand_base_link"},
            },
        }
        with patch.object(model, "_load_config", return_value=config), patch.object(
            model, "_append_o6_models", return_value=None
        ):
            result = model.inject_o6_hands_into_urdf(
                _base_robot_xml(),
                left_o6_can_id="0x28",
                right_o6_can_id="0x27",
                node_namespace="robot",
            )
        root = ET.fromstring(result)

        for side, can_id in (("left", "0x28"), ("right", "0x27")):
            control = root.find(f"./ros2_control[@name='openarmx_{side}_hardware_interface']")
            self.assertEqual(
                control.findtext("./hardware/plugin"),
                "openarmx_hand_hardware/OpenArmXO6System",
            )
            self.assertEqual(control.findtext("./hardware/param[@name='o6_side']"), side)
            self.assertEqual(control.findtext("./hardware/param[@name='o6_can_id']"), can_id)
            self.assertEqual(control.findtext("./hardware/param[@name='node_namespace']"), "robot")
            self.assertEqual(len(control.findall("./joint")), 7)
            self.assertIsNotNone(root.find(f"./joint[@name='openarmx_{side}_link7_to_o6_hand']"))


if __name__ == "__main__":
    unittest.main()
