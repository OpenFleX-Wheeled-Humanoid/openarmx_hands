#pragma once

#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/system_interface.hpp"
#include "openarmx_hand_hardware/o6_async_driver.hpp"
#include "openarmx_hardware/v10_simple_hardware.hpp"

namespace openarmx_hand_hardware
{

class OpenArmXO6System : public hardware_interface::SystemInterface
{
public:
  hardware_interface::CallbackReturn on_init(
    const hardware_interface::HardwareInfo & info) override;
  hardware_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;
  hardware_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;
  hardware_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;
  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;
  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  static constexpr std::size_t kArmJointCount = 7;

  bool parse_parameters(const hardware_interface::HardwareInfo & info);
  hardware_interface::HardwareInfo arm_hardware_info(
    const hardware_interface::HardwareInfo & info) const;
  bool validate_hand_joints(const hardware_interface::HardwareInfo & info) const;

  std::unique_ptr<openarmx_hardware::OpenArmX_v10HW> arm_hardware_;
  std::unique_ptr<O6AsyncDriver> o6_driver_;

  std::string side_;
  std::string arm_prefix_;
  std::string can_interface_;
  int o6_can_id_{0};
  double o6_update_rate_{50.0};
  std::vector<std::string> hand_joint_names_;
  O6Pose hand_position_commands_{};
  O6Pose hand_position_states_{};
};

}  // namespace openarmx_hand_hardware
