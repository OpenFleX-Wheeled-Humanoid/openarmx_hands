import os
import tempfile

import yaml

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    GroupAction,
    OpaqueFunction,
    RegisterEventHandler,
    TimerAction,
)
from launch.conditions import IfCondition
from launch.event_handlers import OnShutdown
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from openarmx_hand_description import build_robot_description


_temporary_urdfs = set()


def _bool(value):
    return value.strip().lower() in ("1", "true", "yes", "on")


def _namespace(context, value):
    return context.perform_substitution(value).strip().strip("/") or None


def _mode(context, value):
    return context.perform_substitution(value).strip().lower()


def _can_id(context, value):
    parsed = int(context.perform_substitution(value), 0)
    if parsed < 0 or parsed > 0x7FF:
        raise RuntimeError(f"O6 CAN ID must be in 0x000..0x7ff, received {parsed:#x}")
    return parsed


def _controller_manager(namespace):
    return f"/{namespace}/controller_manager" if namespace else "/controller_manager"


def _model(
    context,
    hardware_mode,
    use_fake_hardware,
    can_fd,
    left_can,
    right_can,
    left_o6_can_id,
    right_o6_can_id,
    o6_update_rate,
    control_mode,
    namespace,
):
    return build_robot_description(
        o6_hardware_mode=_mode(context, hardware_mode),
        use_fake_hardware=context.perform_substitution(use_fake_hardware),
        can_fd=context.perform_substitution(can_fd),
        left_can_interface=context.perform_substitution(left_can),
        right_can_interface=context.perform_substitution(right_can),
        left_o6_can_id=context.perform_substitution(left_o6_can_id),
        right_o6_can_id=context.perform_substitution(right_o6_can_id),
        o6_update_rate=context.perform_substitution(o6_update_rate),
        control_mode=context.perform_substitution(control_mode),
        node_namespace=_namespace(context, namespace) or "",
    )


def _robot_nodes(
    context,
    hardware_mode,
    use_fake_hardware,
    can_fd,
    left_can,
    right_can,
    left_o6_can_id,
    right_o6_can_id,
    o6_update_rate,
    control_mode,
    namespace_arg,
    controllers,
):
    namespace = _namespace(context, namespace_arg)
    selected_mode = _mode(context, hardware_mode)
    description = _model(
        context,
        hardware_mode,
        use_fake_hardware,
        can_fd,
        left_can,
        right_can,
        left_o6_can_id,
        right_o6_can_id,
        o6_update_rate,
        control_mode,
        namespace_arg,
    )

    nodes = []
    if selected_mode == "independent":
        nodes.append(
            Node(
                package="openarmx_hand_bringup",
                executable="joint_state_merger",
                namespace=namespace,
                output="screen",
            )
        )

    robot_state_remappings = (
        [("joint_states", "openarmx/display/joint_states")]
        if selected_mode == "independent"
        else []
    )
    nodes.extend(
        [
            Node(
                package="robot_state_publisher",
                executable="robot_state_publisher",
                namespace=namespace,
                output="screen",
                parameters=[{"robot_description": description}],
                remappings=robot_state_remappings,
            ),
            Node(
                package="controller_manager",
                executable="ros2_control_node",
                namespace=namespace,
                output="both",
                parameters=[{"robot_description": description}, controllers],
            ),
        ]
    )

    if selected_mode == "shared_bus":
        nodes.append(
            Node(
                package="openarmx_hand_hardware",
                executable="o6_command_adapter",
                namespace=namespace,
                output="screen",
            )
        )
    return nodes


def _driver_remappings(side):
    suffixes = [
        "command",
        "setting_cmd",
        "state",
        "info",
        "force",
        "matrix_touch",
        "matrix_touch_mass",
        "matrix_touch_pc",
        "temperature",
        "current",
        "fault",
        "device_info",
    ]
    base = f"openarmx/o6/{side}"
    return [(f"/{base}/{suffix}", f"{base}/{suffix}") for suffix in suffixes]


def _independent_o6_nodes(
    context,
    hardware_mode,
    use_fake_hardware,
    use_fake_o6_hardware,
    namespace_arg,
    config_path,
    left_o6_can,
    right_o6_can,
    left_o6_can_id,
    right_o6_can_id,
):
    if _mode(context, hardware_mode) != "independent":
        return []

    namespace = _namespace(context, namespace_arg)
    with open(context.perform_substitution(config_path), encoding="utf-8") as stream:
        config = yaml.safe_load(stream)["o6"]

    fake_override = context.perform_substitution(use_fake_o6_hardware).strip()
    fake_o6 = (
        _bool(context.perform_substitution(use_fake_hardware))
        if not fake_override
        else _bool(fake_override)
    )
    input_suffix = "command" if fake_o6 else "state"

    sides = {
        "left": {
            "can": context.perform_substitution(left_o6_can),
            "can_id": _can_id(context, left_o6_can_id),
        },
        "right": {
            "can": context.perform_substitution(right_o6_can),
            "can_id": _can_id(context, right_o6_can_id),
        },
    }
    nodes = []
    if not fake_o6:
        for side, side_config in sides.items():
            parameters = dict(config["driver"])
            parameters.update({"side": side, **side_config})
            nodes.append(
                Node(
                    package="hands_hardware",
                    executable="o6_driver_node",
                    name=f"openarmx_o6_{side}_driver",
                    namespace=namespace,
                    output="screen",
                    parameters=[parameters],
                    remappings=_driver_remappings(side),
                )
            )

    nodes.append(
        Node(
            package="hands_description",
            executable="o6_joint_state_mapper_node",
            name="openarmx_o6_joint_state_mapper",
            namespace=namespace,
            output="screen",
            parameters=[
                {
                    "hand": "both",
                    "left_input_topic": f"openarmx/o6/left/{input_suffix}",
                    "right_input_topic": f"openarmx/o6/right/{input_suffix}",
                    "joint_states_topic": "openarmx/o6/joint_states",
                    **config["mapper"],
                }
            ],
        )
    )
    return nodes


def _spawn_controller(context, namespace_arg, controller):
    namespace = _namespace(context, namespace_arg)
    return [
        Node(
            package="controller_manager",
            executable="spawner",
            namespace=namespace,
            arguments=[controller, "-c", _controller_manager(namespace)],
        )
    ]


def _spawn_arm_controllers(context, namespace_arg, controller_mode):
    selected = context.perform_substitution(controller_mode)
    prefix = (
        "forward_position"
        if selected == "forward_position_controller"
        else "joint_trajectory"
    )
    namespace = _namespace(context, namespace_arg)
    return [
        Node(
            package="controller_manager",
            executable="spawner",
            namespace=namespace,
            arguments=[
                f"left_{prefix}_controller",
                f"right_{prefix}_controller",
                "-c",
                _controller_manager(namespace),
            ],
        )
    ]


def _spawn_hand_controllers(context, hardware_mode, namespace_arg):
    if _mode(context, hardware_mode) != "shared_bus":
        return []
    return [
        *_spawn_controller(context, namespace_arg, "left_o6_position_controller"),
        *_spawn_controller(context, namespace_arg, "right_o6_position_controller"),
    ]


def _validate_configuration(
    context,
    hardware_mode,
    use_fake_hardware,
    use_fake_o6_hardware,
    can_fd,
    left_can,
    right_can,
    left_o6_can,
    right_o6_can,
    enabled_effort,
    control_mode,
):
    selected_mode = _mode(context, hardware_mode)
    arm_fake = _bool(context.perform_substitution(use_fake_hardware))
    hand_fake_override = context.perform_substitution(use_fake_o6_hardware).strip()

    if selected_mode == "shared_bus":
        if _bool(context.perform_substitution(can_fd)):
            raise RuntimeError("shared_bus mode requires can_fd:=false")
        if hand_fake_override and _bool(hand_fake_override) != arm_fake:
            raise RuntimeError(
                "shared_bus mode cannot select fake arm and O6 hardware independently"
            )
    else:
        arm_interfaces = {
            context.perform_substitution(left_can),
            context.perform_substitution(right_can),
        }
        hand_interfaces = {
            context.perform_substitution(left_o6_can),
            context.perform_substitution(right_o6_can),
        }
        overlap = arm_interfaces & hand_interfaces
        if overlap:
            raise RuntimeError(
                "independent mode requires separate arm and O6 CAN interfaces; "
                f"overlap: {', '.join(sorted(overlap))}"
            )

    if _bool(context.perform_substitution(enabled_effort)) and (
        context.perform_substitution(control_mode) != "mit"
    ):
        raise RuntimeError("Gravity compensation requires control_mode:=mit")
    return []


def _gravity_node(
    context,
    hardware_mode,
    use_fake_hardware,
    can_fd,
    left_can,
    right_can,
    left_o6_can_id,
    right_o6_can_id,
    o6_update_rate,
    control_mode,
    namespace_arg,
):
    namespace = _namespace(context, namespace_arg)
    description = _model(
        context,
        hardware_mode,
        use_fake_hardware,
        can_fd,
        left_can,
        right_can,
        left_o6_can_id,
        right_o6_can_id,
        o6_update_rate,
        control_mode,
        namespace_arg,
    )
    descriptor, path = tempfile.mkstemp(
        prefix=f"openarmx_o6_{namespace or 'default'}_", suffix=".urdf"
    )
    with os.fdopen(descriptor, "w", encoding="utf-8") as stream:
        stream.write(description)
    _temporary_urdfs.add(path)
    return [
        Node(
            package="openarmx_gravity_comp",
            executable="gravity_comp_node",
            namespace=namespace,
            output="screen",
            parameters=[
                {
                    "urdf_path": path,
                    "g_scale": 1.05,
                    "enable_left": True,
                    "enable_right": True,
                    "verbose": False,
                }
            ],
            remappings=[
                ("/joint_states", "joint_states"),
                (
                    "/left_forward_effort_controller/commands",
                    "left_forward_effort_controller/commands",
                ),
                (
                    "/right_forward_effort_controller/commands",
                    "right_forward_effort_controller/commands",
                ),
            ],
        )
    ]


def _cleanup(_context):
    for path in list(_temporary_urdfs):
        try:
            os.unlink(path)
        except FileNotFoundError:
            pass
        _temporary_urdfs.discard(path)
    return []


def generate_launch_description():
    arguments = [
        DeclareLaunchArgument(
            "o6_hardware_mode",
            default_value="shared_bus",
            choices=["shared_bus", "independent"],
        ),
        DeclareLaunchArgument("use_fake_hardware", default_value="false"),
        DeclareLaunchArgument("use_fake_o6_hardware", default_value=""),
        DeclareLaunchArgument("robot_namespace", default_value=""),
        DeclareLaunchArgument("left_can_interface", default_value="can1"),
        DeclareLaunchArgument("right_can_interface", default_value="can0"),
        DeclareLaunchArgument("left_o6_can_interface", default_value="can3"),
        DeclareLaunchArgument("right_o6_can_interface", default_value="can2"),
        DeclareLaunchArgument("left_o6_can_id", default_value="0x28"),
        DeclareLaunchArgument("right_o6_can_id", default_value="0x27"),
        DeclareLaunchArgument("o6_update_rate", default_value="50.0"),
        DeclareLaunchArgument("can_fd", default_value="false"),
        DeclareLaunchArgument(
            "control_mode", default_value="mit", choices=["mit", "csp"]
        ),
        DeclareLaunchArgument(
            "robot_controller",
            default_value="joint_trajectory_controller",
            choices=["joint_trajectory_controller", "forward_position_controller"],
        ),
        DeclareLaunchArgument("enable_forward_effort", default_value="false"),
        DeclareLaunchArgument("launch_rviz", default_value="true"),
    ]

    hardware_mode = LaunchConfiguration("o6_hardware_mode")
    fake_arm = LaunchConfiguration("use_fake_hardware")
    fake_o6 = LaunchConfiguration("use_fake_o6_hardware")
    namespace = LaunchConfiguration("robot_namespace")
    left_can = LaunchConfiguration("left_can_interface")
    right_can = LaunchConfiguration("right_can_interface")
    left_o6_can = LaunchConfiguration("left_o6_can_interface")
    right_o6_can = LaunchConfiguration("right_o6_can_interface")
    left_o6_can_id = LaunchConfiguration("left_o6_can_id")
    right_o6_can_id = LaunchConfiguration("right_o6_can_id")
    o6_update_rate = LaunchConfiguration("o6_update_rate")
    can_fd = LaunchConfiguration("can_fd")
    control_mode = LaunchConfiguration("control_mode")
    robot_controller = LaunchConfiguration("robot_controller")
    effort = LaunchConfiguration("enable_forward_effort")
    launch_rviz = LaunchConfiguration("launch_rviz")
    share = FindPackageShare("openarmx_hand_bringup")
    controllers = PathJoinSubstitution([share, "config", "controllers.yaml"])
    o6_config = PathJoinSubstitution([share, "config", "o6.yaml"])

    validate = OpaqueFunction(
        function=_validate_configuration,
        args=[
            hardware_mode,
            fake_arm,
            fake_o6,
            can_fd,
            left_can,
            right_can,
            left_o6_can,
            right_o6_can,
            effort,
            control_mode,
        ],
    )
    robot_nodes = OpaqueFunction(
        function=_robot_nodes,
        args=[
            hardware_mode,
            fake_arm,
            can_fd,
            left_can,
            right_can,
            left_o6_can_id,
            right_o6_can_id,
            o6_update_rate,
            control_mode,
            namespace,
            controllers,
        ],
    )
    independent_o6_nodes = OpaqueFunction(
        function=_independent_o6_nodes,
        args=[
            hardware_mode,
            fake_arm,
            fake_o6,
            namespace,
            o6_config,
            left_o6_can,
            right_o6_can,
            left_o6_can_id,
            right_o6_can_id,
        ],
    )
    joint_state_broadcaster = OpaqueFunction(
        function=_spawn_controller,
        args=[namespace, "joint_state_broadcaster"],
    )
    arm_controllers = OpaqueFunction(
        function=_spawn_arm_controllers,
        args=[namespace, robot_controller],
    )
    hand_controllers = OpaqueFunction(
        function=_spawn_hand_controllers,
        args=[hardware_mode, namespace],
    )
    effort_controllers = OpaqueFunction(
        function=lambda context: [
            *_spawn_controller(context, namespace, "left_forward_effort_controller"),
            *_spawn_controller(context, namespace, "right_forward_effort_controller"),
        ]
    )
    gravity = OpaqueFunction(
        function=_gravity_node,
        args=[
            hardware_mode,
            fake_arm,
            can_fd,
            left_can,
            right_can,
            left_o6_can_id,
            right_o6_can_id,
            o6_update_rate,
            control_mode,
            namespace,
        ],
    )
    rviz = OpaqueFunction(
        function=lambda context: [
            Node(
                package="rviz2",
                executable="rviz2",
                namespace=_namespace(context, namespace),
                output="log",
                arguments=[
                    "-d",
                    PathJoinSubstitution([share, "rviz", "bimanual_o6.rviz"]),
                ],
                condition=IfCondition(launch_rviz),
            )
        ]
    )

    return LaunchDescription(
        arguments
        + [
            validate,
            robot_nodes,
            independent_o6_nodes,
            rviz,
            TimerAction(
                period=1.0,
                actions=[joint_state_broadcaster, arm_controllers, hand_controllers],
            ),
            GroupAction(
                condition=IfCondition(effort),
                actions=[
                    TimerAction(period=1.0, actions=[effort_controllers]),
                    TimerAction(period=2.0, actions=[gravity]),
                ],
            ),
            RegisterEventHandler(
                OnShutdown(on_shutdown=[OpaqueFunction(function=_cleanup)])
            ),
        ]
    )
