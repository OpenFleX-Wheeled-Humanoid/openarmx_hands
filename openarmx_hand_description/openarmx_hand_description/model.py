"""Build an O6-equipped robot model without modifying upstream description packages."""

from copy import deepcopy
import os
import xml.etree.ElementTree as ET

from ament_index_python.packages import get_package_share_directory
import xacro
import yaml


def _load_config():
    share = get_package_share_directory("openarmx_hand_description")
    with open(os.path.join(share, "config", "model.yaml"), encoding="utf-8") as stream:
        return yaml.safe_load(stream)


def _render_upstream_model(mappings):
    description_share = get_package_share_directory("openarmx_description")
    xacro_path = os.path.join(description_share, "urdf", "robot", "v10.urdf.xacro")
    upstream_mappings = {
        "arm_type": "v10",
        "bimanual": "true",
        "ee_type": "none",
        "hand": "false",
        "ros2_control": "true",
        "use_fake_hardware": "false",
        "fake_sensor_commands": "false",
        "left_can_interface": "can1",
        "right_can_interface": "can0",
        "can_fd": "false",
        "control_mode": "mit",
    }
    upstream_mappings.update({key: str(value) for key, value in mappings.items()})
    return xacro.process_file(xacro_path, mappings=upstream_mappings).toxml()


def _add_o6_control_joints(control, side):
    model_prefix = "lh" if side == "left" else "rh"
    suffixes = (
        "thumb_cmc_pitch",
        "thumb_cmc_yaw",
        "index_mcp_pitch",
        "middle_mcp_pitch",
        "ring_mcp_pitch",
        "pinky_mcp_pitch",
    )
    initial_positions = (0.125098, 0.0, 0.0, 0.0, 0.0, 0.0)
    for suffix, initial_position in zip(suffixes, initial_positions):
        joint = ET.SubElement(
            control,
            "joint",
            {"name": f"{side}_{model_prefix}_{suffix}"},
        )
        ET.SubElement(joint, "command_interface", {"name": "position"})
        state = ET.SubElement(joint, "state_interface", {"name": "position"})
        initial = ET.SubElement(state, "param", {"name": "initial_value"})
        initial.text = str(initial_position)


def _configure_o6_hardware(root, side, can_id, update_rate):
    control = root.find(f"./ros2_control[@name='openarmx_{side}_hardware_interface']")
    if control is None:
        raise RuntimeError(f"Upstream model is missing {side} ros2_control system")

    hardware = control.find("hardware")
    plugin = hardware.find("plugin") if hardware is not None else None
    if plugin is None:
        raise RuntimeError(f"Upstream {side} ros2_control system has no hardware plugin")

    if plugin.text == "openarmx_hardware/OpenArmX_v10HW":
        plugin.text = "openarmx_hand_hardware/OpenArmXO6System"
        for name, value in (
            ("o6_side", side),
            ("o6_can_id", can_id),
            ("o6_update_rate", update_rate),
        ):
            parameter = ET.SubElement(hardware, "param", {"name": name})
            parameter.text = str(value)

    _add_o6_control_joints(control, side)


def _patch_arm(
    root,
    config,
    node_namespace,
    o6_hardware_mode,
    left_o6_can_id,
    right_o6_can_id,
    o6_update_rate,
):
    visual_uri = config["link7"]["visual_mesh"]
    collision_uri = config["link7"]["collision_mesh"]
    link7_mass = str(config["link7"]["mass"])

    for side in ("left", "right"):
        prefix = f"openarmx_{side}_"
        link7 = root.find(f"./link[@name='{prefix}link7']")
        if link7 is None:
            raise RuntimeError(f"Upstream model is missing {prefix}link7")

        visual_mesh = link7.find("./visual/geometry/mesh")
        collision_mesh = link7.find("./collision/geometry/mesh")
        mass = link7.find("./inertial/mass")
        if visual_mesh is None or collision_mesh is None or mass is None:
            raise RuntimeError(
                f"Upstream {prefix}link7 has no visual mesh, collision mesh, or inertial mass"
            )
        visual_mesh.set("filename", visual_uri)
        collision_mesh.set("filename", collision_uri)
        mass.set("value", link7_mass)

        pico_joint = root.find(f"./joint[@name='{prefix}link7_pico_joint']")
        if pico_joint is None:
            raise RuntimeError(f"Upstream model is missing {prefix}link7_pico_joint")
        pico = config["tcp"][side]
        pico_joint.find("origin").set("xyz", pico["xyz"])
        pico_joint.find("origin").set("rpy", pico["rpy"])

    if o6_hardware_mode == "shared_bus":
        _configure_o6_hardware(root, "left", left_o6_can_id, o6_update_rate)
        _configure_o6_hardware(root, "right", right_o6_can_id, o6_update_rate)

    if node_namespace:
        for control in root.findall("./ros2_control"):
            hardware = control.find("hardware")
            plugin = hardware.find("plugin") if hardware is not None else None
            if plugin is None or plugin.text not in (
                "openarmx_hardware/OpenArmX_v10HW",
                "openarmx_hand_hardware/OpenArmXO6System",
            ):
                continue
            parameter = ET.SubElement(hardware, "param", {"name": "node_namespace"})
            parameter.text = node_namespace


def _append_o6_models(root):
    hands_share = get_package_share_directory("hands_description")
    paths = [
        os.path.join(
            hands_share,
            "urdf",
            "o6",
            side,
            f"linkerhand_o6_{side}_prefixed.urdf.xacro",
        )
        for side in ("left", "right")
    ]
    for path in paths:
        hand_root = ET.parse(path).getroot()
        for child in hand_root:
            root.append(deepcopy(child))


def _append_mounts(root, config):
    for side in ("left", "right"):
        mount = config["mount"][side]
        joint = ET.SubElement(
            root,
            "joint",
            {"name": f"openarmx_{side}_link7_to_o6_hand", "type": "fixed"},
        )
        ET.SubElement(joint, "origin", {"xyz": mount["xyz"], "rpy": mount["rpy"]})
        ET.SubElement(joint, "parent", {"link": mount["parent"]})
        ET.SubElement(joint, "child", {"link": mount["child"]})


def inject_o6_hands_into_urdf(
    urdf_xml,
    *,
    o6_hardware_mode="shared_bus",
    left_o6_can_id="0x28",
    right_o6_can_id="0x27",
    o6_update_rate="50.0",
    node_namespace="",
):
    """Attach both O6 hands and their shared-bus control interfaces to a robot URDF.

    The input must already contain the two OpenArmX arm ros2_control systems and
    the `openarmx_{left,right}_link7` plus Pico TCP links. This makes the O6
    integration reusable by the standalone bimanual model and the integrated
    chassis/lift robot model.
    """
    mode = str(o6_hardware_mode).strip().lower()
    if mode not in ("shared_bus", "independent"):
        raise ValueError(
            "o6_hardware_mode must be 'shared_bus' or 'independent', "
            f"received {o6_hardware_mode!r}"
        )

    config = _load_config()
    root = ET.fromstring(urdf_xml)
    _patch_arm(
        root,
        config,
        node_namespace.strip("/"),
        mode,
        left_o6_can_id,
        right_o6_can_id,
        o6_update_rate,
    )
    _append_o6_models(root)
    _append_mounts(root, config)
    ET.indent(root, space="  ")
    return ET.tostring(root, encoding="unicode", xml_declaration=True)


def build_robot_description(*, node_namespace="", **mappings):
    """Return a complete bimanual arm and O6 URDF XML string."""
    o6_hardware_mode = str(mappings.pop("o6_hardware_mode", "shared_bus")).strip().lower()
    left_o6_can_id = mappings.pop("left_o6_can_id", "0x28")
    right_o6_can_id = mappings.pop("right_o6_can_id", "0x27")
    o6_update_rate = mappings.pop("o6_update_rate", "50.0")
    return inject_o6_hands_into_urdf(
        _render_upstream_model(mappings),
        o6_hardware_mode=o6_hardware_mode,
        left_o6_can_id=left_o6_can_id,
        right_o6_can_id=right_o6_can_id,
        o6_update_rate=o6_update_rate,
        node_namespace=node_namespace,
    )
