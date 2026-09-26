from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument("hand", default_value="both", description="left | right | both"),
        DeclareLaunchArgument("publish_rate", default_value="100.0"),
        DeclareLaunchArgument("left_input_topic", default_value="/higvr/left_hand"),
        DeclareLaunchArgument("right_input_topic", default_value="/higvr/right_hand"),
        DeclareLaunchArgument("left_output_topic", default_value="/openarmx/o6/left/command"),
        DeclareLaunchArgument("right_output_topic", default_value="/openarmx/o6/right/command"),
        DeclareLaunchArgument("thumb_close_value", default_value="0.0"),
        DeclareLaunchArgument("finger_close_value", default_value="0.0"),
        DeclareLaunchArgument("thumb_yaw_min", default_value="0.0"),
        DeclareLaunchArgument("thumb_yaw_max", default_value="255.0"),
        DeclareLaunchArgument("pinky_open_value", default_value="255.0"),
        Node(
            package="openarmx_hands_bridge",
            executable="higvr_to_o6_node",
            name="higvr_to_o6_node",
            output="screen",
            parameters=[{
                "hand": LaunchConfiguration("hand"),
                "publish_rate": LaunchConfiguration("publish_rate"),
                "left_input_topic": LaunchConfiguration("left_input_topic"),
                "right_input_topic": LaunchConfiguration("right_input_topic"),
                "left_output_topic": LaunchConfiguration("left_output_topic"),
                "right_output_topic": LaunchConfiguration("right_output_topic"),
                "thumb_close_value": LaunchConfiguration("thumb_close_value"),
                "finger_close_value": LaunchConfiguration("finger_close_value"),
                "thumb_yaw_min": LaunchConfiguration("thumb_yaw_min"),
                "thumb_yaw_max": LaunchConfiguration("thumb_yaw_max"),
                "pinky_open_value": LaunchConfiguration("pinky_open_value"),
            }],
        ),
    ])
