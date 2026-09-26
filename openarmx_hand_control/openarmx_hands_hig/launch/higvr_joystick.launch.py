from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument("port", default_value="auto"),
        DeclareLaunchArgument("baud_rate", default_value="115200"),
        DeclareLaunchArgument("hand_mode", default_value="both"),
        DeclareLaunchArgument("left_hand_id", default_value="1"),
        DeclareLaunchArgument("right_hand_id", default_value="2"),
        DeclareLaunchArgument("publish_raw", default_value="false"),
        DeclareLaunchArgument("publish_vr_topics", default_value="true"),
        DeclareLaunchArgument("joystick_center", default_value="510"),
        DeclareLaunchArgument("joystick_deadzone", default_value="100"),
        DeclareLaunchArgument("joystick_minimum", default_value="0"),
        DeclareLaunchArgument("joystick_maximum", default_value="1023"),
        DeclareLaunchArgument("joystick_invert_x", default_value="true"),
        DeclareLaunchArgument("joystick_invert_y", default_value="false"),
        DeclareLaunchArgument("joystick_invert_left_y", default_value="true"),
        DeclareLaunchArgument("joystick_invert_right_y", default_value="false"),
        DeclareLaunchArgument("auto_fix_permissions", default_value="true"),
        DeclareLaunchArgument("reverse_left_fingers", default_value="true"),
        DeclareLaunchArgument("reverse_right_fingers", default_value="false"),
        Node(
            package="openarmx_hands_hig",
            executable="higvr_hands_joystick",
            name="higvr_hands_joystick",
            output="screen",
            parameters=[{
                "port": LaunchConfiguration("port"),
                "baud_rate": ParameterValue(LaunchConfiguration("baud_rate"), value_type=int),
                "hand_mode": LaunchConfiguration("hand_mode"),
                "left_hand_id": ParameterValue(LaunchConfiguration("left_hand_id"), value_type=int),
                "right_hand_id": ParameterValue(LaunchConfiguration("right_hand_id"), value_type=int),
                "publish_raw": ParameterValue(LaunchConfiguration("publish_raw"), value_type=bool),
                "publish_vr_topics": ParameterValue(
                    LaunchConfiguration("publish_vr_topics"), value_type=bool),
                "joystick_center": ParameterValue(
                    LaunchConfiguration("joystick_center"), value_type=int),
                "joystick_deadzone": ParameterValue(
                    LaunchConfiguration("joystick_deadzone"), value_type=int),
                "joystick_minimum": ParameterValue(
                    LaunchConfiguration("joystick_minimum"), value_type=int),
                "joystick_maximum": ParameterValue(
                    LaunchConfiguration("joystick_maximum"), value_type=int),
                "joystick_invert_x": ParameterValue(
                    LaunchConfiguration("joystick_invert_x"), value_type=bool),
                "joystick_invert_y": ParameterValue(
                    LaunchConfiguration("joystick_invert_y"), value_type=bool),
                "joystick_invert_left_y": ParameterValue(
                    LaunchConfiguration("joystick_invert_left_y"), value_type=bool),
                "joystick_invert_right_y": ParameterValue(
                    LaunchConfiguration("joystick_invert_right_y"), value_type=bool),
                "auto_fix_permissions": ParameterValue(
                    LaunchConfiguration("auto_fix_permissions"), value_type=bool),
                "reverse_left_fingers": ParameterValue(
                    LaunchConfiguration("reverse_left_fingers"), value_type=bool),
                "reverse_right_fingers": ParameterValue(
                    LaunchConfiguration("reverse_right_fingers"), value_type=bool),
            }],
        ),
    ])
