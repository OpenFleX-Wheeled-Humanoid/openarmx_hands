# openarmx_hand_hardware

Specialized ros2_control hardware for an OpenArmX V10 arm and a LinkerHand O6
sharing one classic CAN bus. Each plugin instance exposes seven arm joints and
six actuated O6 joints. The existing OpenArmX arm hardware implementation is
composed internally and remains unchanged.

The O6 transport runs in a background thread so its protocol delays do not
block the controller manager update loop. `o6_command_adapter` preserves the
legacy `/openarmx/o6/<side>/command` and `/state` JointState topics.
