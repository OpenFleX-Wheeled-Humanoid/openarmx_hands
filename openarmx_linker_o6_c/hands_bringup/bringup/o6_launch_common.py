from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def launch_arguments():
    from launch.actions import DeclareLaunchArgument

    return [
        DeclareLaunchArgument("sim", default_value="true", description="true: only RViz/model, false: real hardware driver"),
        DeclareLaunchArgument("hand", default_value="both", description="left | right | both"),
        DeclareLaunchArgument("left_can", default_value="can0"),
        DeclareLaunchArgument("right_can", default_value="can1"),
        DeclareLaunchArgument("left_can_id", default_value="40"),
        DeclareLaunchArgument("right_can_id", default_value="39"),
        DeclareLaunchArgument("transport", default_value="can", description="can | rs485"),
        DeclareLaunchArgument("left_modbus_port", default_value="/dev/ttyUSB0"),
        DeclareLaunchArgument("right_modbus_port", default_value="/dev/ttyUSB1"),
        DeclareLaunchArgument("left_modbus_id", default_value="40"),
        DeclareLaunchArgument("right_modbus_id", default_value="39"),
        DeclareLaunchArgument("modbus_baudrate", default_value="115200"),
        DeclareLaunchArgument("driver_rate", default_value="100.0"),
        DeclareLaunchArgument("publish_rate", default_value="50.0"),
        DeclareLaunchArgument("info_rate", default_value="1.0"),
        DeclareLaunchArgument("poll_touch", default_value="false"),
        DeclareLaunchArgument("poll_matrix_touch", default_value="false"),
        DeclareLaunchArgument("poll_diagnostics", default_value="true"),
        DeclareLaunchArgument("poll_device_info", default_value="true"),
        DeclareLaunchArgument("launch_rviz", default_value="true"),
    ]


def driver_node(side):
    return Node(
        package="hands_hardware",
        executable="o6_driver_node",
        name=f"openarmx_o6_c_{side}_driver",
        output="screen",
        parameters=[{
            "side": side,
            "transport": LaunchConfiguration("transport"),
            "can": LaunchConfiguration(f"{side}_can"),
            "can_id": LaunchConfiguration(f"{side}_can_id"),
            "modbus_port": LaunchConfiguration(f"{side}_modbus_port"),
            "modbus_id": LaunchConfiguration(f"{side}_modbus_id"),
            "modbus_baudrate": LaunchConfiguration("modbus_baudrate"),
            "publish_rate": LaunchConfiguration("driver_rate"),
            "info_rate": LaunchConfiguration("info_rate"),
            "poll_touch": LaunchConfiguration("poll_touch"),
            "poll_matrix_touch": LaunchConfiguration("poll_matrix_touch"),
            "poll_diagnostics": LaunchConfiguration("poll_diagnostics"),
            "poll_device_info": LaunchConfiguration("poll_device_info"),
        }],
    )


def _as_bool(value):
    return value.strip().lower() in ("1", "true", "yes", "on")


def bringup_nodes(context):
    from launch.conditions import IfCondition

    hand = LaunchConfiguration("hand").perform(context).strip().lower()
    if hand not in ("left", "right", "both"):
        raise RuntimeError("hand must be one of: left, right, both")

    sim = _as_bool(LaunchConfiguration("sim").perform(context))
    left_input = "/openarmx/o6/left/command" if sim else "/openarmx/o6/left/state"
    right_input = "/openarmx/o6/right/command" if sim else "/openarmx/o6/right/state"

    nodes = []
    if not sim:
        if hand in ("left", "both"):
            nodes.append(driver_node("left"))
        if hand in ("right", "both"):
            nodes.append(driver_node("right"))

    nodes.extend(model_nodes(hand, left_input=left_input, right_input=right_input))
    nodes.append(rviz_node(condition=IfCondition(LaunchConfiguration("launch_rviz"))))
    return nodes


def model_nodes(hand, left_input=None, right_input=None):
    package_share = FindPackageShare("hands_description")
    description = ParameterValue(
        Command([
            PathJoinSubstitution([package_share, "../../lib/hands_description/o6_robot_description_cli"]),
            " ",
            package_share,
            " ",
            hand,
        ]),
        value_type=str,
    )
    return [
        Node(
            package="robot_state_publisher",
            executable="robot_state_publisher",
            name="openarmx_o6_c_robot_state_publisher",
            output="screen",
            parameters=[{"robot_description": description}],
        ),
        Node(
            package="hands_description",
            executable="o6_joint_state_mapper_node",
            name="openarmx_o6_c_joint_state_mapper",
            output="screen",
            parameters=[{
                "hand": hand,
                "left_input_topic": left_input or "/openarmx/o6/left/command",
                "right_input_topic": right_input or "/openarmx/o6/right/command",
                "publish_rate": LaunchConfiguration("publish_rate"),
            }],
        ),
    ]


def rviz_node(condition=None):
    return Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=["-d", PathJoinSubstitution([FindPackageShare("hands_description"), "rviz", "o6.rviz"])],
        condition=condition,
    )
