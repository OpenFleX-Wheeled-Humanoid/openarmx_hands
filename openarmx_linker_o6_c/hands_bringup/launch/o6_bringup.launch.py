import os
import sys

from launch import LaunchDescription
from launch.actions import OpaqueFunction

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "bringup"))

from o6_launch_common import bringup_nodes, launch_arguments


def generate_launch_description():
    return LaunchDescription([
        *launch_arguments(),
        OpaqueFunction(function=bringup_nodes),
    ])
