#include "openarmx_hand_hardware/openarmx_o6_system.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <utility>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/logging.hpp"

namespace openarmx_hand_hardware
{
namespace
{

std::string lower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(),
    [](unsigned char character) {return static_cast<char>(std::tolower(character));});
  return value;
}

}  // namespace

bool OpenArmXO6System::parse_parameters(const hardware_interface::HardwareInfo & info)
{
  const auto parameter = [&info](const std::string & name, const std::string & fallback) {
      const auto found = info.hardware_parameters.find(name);
      return found == info.hardware_parameters.end() ? fallback : found->second;
    };

  side_ = lower(parameter("o6_side", ""));
  arm_prefix_ = parameter("arm_prefix", side_.empty() ? "" : side_ + "_");
  can_interface_ = parameter("can_interface", side_ == "left" ? "can1" : "can0");

  try {
    o6_can_id_ = std::stoi(parameter("o6_can_id", side_ == "left" ? "40" : "39"), nullptr, 0);
    o6_update_rate_ = std::stod(parameter("o6_update_rate", "50.0"));
  } catch (const std::exception & exception) {
    RCLCPP_ERROR(
      rclcpp::get_logger("OpenArmXO6System"), "Invalid O6 parameter: %s", exception.what());
    return false;
  }

  if (side_ != "left" && side_ != "right") {
    RCLCPP_ERROR(
      rclcpp::get_logger("OpenArmXO6System"),
      "o6_side must be 'left' or 'right', received '%s'", side_.c_str());
    return false;
  }
  if (o6_can_id_ < 0 || o6_can_id_ > 0x7FF) {
    RCLCPP_ERROR(
      rclcpp::get_logger("OpenArmXO6System"), "Invalid O6 CAN ID: %d", o6_can_id_);
    return false;
  }

  const auto can_fd = lower(parameter("can_fd", "false"));
  if (can_fd == "true" || can_fd == "1") {
    RCLCPP_ERROR(
      rclcpp::get_logger("OpenArmXO6System"),
      "Shared arm/O6 CAN requires can_fd=false");
    return false;
  }
  return true;
}

hardware_interface::HardwareInfo OpenArmXO6System::arm_hardware_info(
  const hardware_interface::HardwareInfo & info) const
{
  auto arm_info = info;
  const auto arm_joint_prefix = "openarmx_" + arm_prefix_ + "joint";
  arm_info.joints.erase(
    std::remove_if(
      arm_info.joints.begin(), arm_info.joints.end(),
      [&arm_joint_prefix](const auto & joint) {
        return joint.name.rfind(arm_joint_prefix, 0) != 0;
      }),
    arm_info.joints.end());
  arm_info.hardware_parameters["hand"] = "false";
  return arm_info;
}

bool OpenArmXO6System::validate_hand_joints(const hardware_interface::HardwareInfo & info) const
{
  for (const auto & expected : hand_joint_names_) {
    const auto found = std::find_if(
      info.joints.begin(), info.joints.end(),
      [&expected](const auto & joint) {return joint.name == expected;});
    if (found == info.joints.end()) {
      RCLCPP_ERROR(
        rclcpp::get_logger("OpenArmXO6System"),
        "Missing O6 ros2_control joint '%s'", expected.c_str());
      return false;
    }
  }
  return true;
}

hardware_interface::CallbackReturn OpenArmXO6System::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }
  if (!parse_parameters(info)) {
    return CallbackReturn::ERROR;
  }

  hand_joint_names_ = o6_joint_names(side_);
  if (!validate_hand_joints(info)) {
    return CallbackReturn::ERROR;
  }

  auto arm_info = arm_hardware_info(info);
  if (arm_info.joints.size() != kArmJointCount) {
    RCLCPP_ERROR(
      rclcpp::get_logger("OpenArmXO6System"),
      "Expected 7 arm joints for prefix '%s', found %zu",
      arm_prefix_.c_str(), arm_info.joints.size());
    return CallbackReturn::ERROR;
  }

  arm_hardware_ = std::make_unique<openarmx_hardware::OpenArmX_v10HW>();
  if (arm_hardware_->on_init(arm_info) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }

  const auto open_radians = o6_raw_to_radians(o6_open_raw_pose(), side_);
  hand_position_commands_ = open_radians;
  hand_position_states_ = open_radians;
  o6_driver_ = std::make_unique<O6AsyncDriver>(
    can_interface_, o6_can_id_, o6_update_rate_);

  RCLCPP_INFO(
    rclcpp::get_logger("OpenArmXO6System"),
    "Configured %s arm and O6 on %s (O6 CAN ID 0x%x)",
    side_.c_str(), can_interface_.c_str(), o6_can_id_);
  return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn OpenArmXO6System::on_configure(
  const rclcpp_lifecycle::State & previous_state)
{
  return arm_hardware_->on_configure(previous_state);
}

hardware_interface::CallbackReturn OpenArmXO6System::on_activate(
  const rclcpp_lifecycle::State & previous_state)
{
  if (arm_hardware_->on_activate(previous_state) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }
  if (!o6_driver_->start()) {
    RCLCPP_ERROR(
      rclcpp::get_logger("OpenArmXO6System"),
      "Failed to start %s O6: %s", side_.c_str(), o6_driver_->last_error().c_str());
    arm_hardware_->on_deactivate(previous_state);
    return CallbackReturn::ERROR;
  }
  o6_driver_->set_command(o6_radians_to_raw(hand_position_commands_, side_));
  return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn OpenArmXO6System::on_deactivate(
  const rclcpp_lifecycle::State & previous_state)
{
  o6_driver_->stop();
  return arm_hardware_->on_deactivate(previous_state);
}

std::vector<hardware_interface::StateInterface> OpenArmXO6System::export_state_interfaces()
{
  auto interfaces = arm_hardware_->export_state_interfaces();
  interfaces.reserve(interfaces.size() + kO6JointCount);
  for (std::size_t index = 0; index < kO6JointCount; ++index) {
    interfaces.emplace_back(
      hand_joint_names_[index], hardware_interface::HW_IF_POSITION,
      &hand_position_states_[index]);
  }
  return interfaces;
}

std::vector<hardware_interface::CommandInterface> OpenArmXO6System::export_command_interfaces()
{
  auto interfaces = arm_hardware_->export_command_interfaces();
  interfaces.reserve(interfaces.size() + kO6JointCount);
  for (std::size_t index = 0; index < kO6JointCount; ++index) {
    interfaces.emplace_back(
      hand_joint_names_[index], hardware_interface::HW_IF_POSITION,
      &hand_position_commands_[index]);
  }
  return interfaces;
}

hardware_interface::return_type OpenArmXO6System::read(
  const rclcpp::Time & time, const rclcpp::Duration & period)
{
  const auto arm_result = arm_hardware_->read(time, period);
  if (arm_result != hardware_interface::return_type::OK) {
    return arm_result;
  }
  if (o6_driver_->has_state()) {
    hand_position_states_ = o6_raw_to_radians(o6_driver_->state(), side_);
  }
  return hardware_interface::return_type::OK;
}

hardware_interface::return_type OpenArmXO6System::write(
  const rclcpp::Time & time, const rclcpp::Duration & period)
{
  const auto arm_result = arm_hardware_->write(time, period);
  if (arm_result != hardware_interface::return_type::OK) {
    return arm_result;
  }
  o6_driver_->set_command(o6_radians_to_raw(hand_position_commands_, side_));
  return hardware_interface::return_type::OK;
}

}  // namespace openarmx_hand_hardware

PLUGINLIB_EXPORT_CLASS(
  openarmx_hand_hardware::OpenArmXO6System,
  hardware_interface::SystemInterface)
