from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    port = LaunchConfiguration("port")
    baud_rate = LaunchConfiguration("baud_rate")
    publish_raw = LaunchConfiguration("publish_raw")
    auto_fix_permissions = LaunchConfiguration("auto_fix_permissions")
    reverse_left_fingers = LaunchConfiguration("reverse_left_fingers")
    reverse_right_fingers = LaunchConfiguration("reverse_right_fingers")

    return LaunchDescription(
        [
            DeclareLaunchArgument("port", default_value="auto"),
            DeclareLaunchArgument("baud_rate", default_value="115200"),
            DeclareLaunchArgument("publish_raw", default_value="false"),
            DeclareLaunchArgument("auto_fix_permissions", default_value="true"),
            DeclareLaunchArgument("reverse_left_fingers", default_value="true"),
            DeclareLaunchArgument("reverse_right_fingers", default_value="false"),
            Node(
                package="openarmx_hands_hig",
                executable="higvr_hands_node",
                name="higvr_both_hands_node",
                output="screen",
                parameters=[
                    {
                        "port": port,
                        "baud_rate": ParameterValue(baud_rate, value_type=int),
                        "hand_mode": "both",
                        "publish_raw": ParameterValue(publish_raw, value_type=bool),
                        "auto_fix_permissions": ParameterValue(
                            auto_fix_permissions, value_type=bool
                        ),
                        "reverse_left_fingers": ParameterValue(
                            reverse_left_fingers, value_type=bool
                        ),
                        "reverse_right_fingers": ParameterValue(
                            reverse_right_fingers, value_type=bool
                        ),
                    }
                ],
            ),
        ]
    )
