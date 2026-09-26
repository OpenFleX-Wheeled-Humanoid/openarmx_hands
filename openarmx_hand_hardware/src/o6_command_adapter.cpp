#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "openarmx_hand_hardware/o6_joint_mapping.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

namespace openarmx_hand_hardware
{

class O6CommandAdapter : public rclcpp::Node
{
public:
  O6CommandAdapter()
  : Node("openarmx_o6_command_adapter")
  {
    joint_states_topic_ = declare_parameter<std::string>("joint_states_topic", "joint_states");
    for (const auto & side : {std::string("left"), std::string("right")}) {
      const auto command_topic = declare_parameter<std::string>(
        side + "_command_topic", "openarmx/o6/" + side + "/command");
      const auto state_topic = declare_parameter<std::string>(
        side + "_state_topic", "openarmx/o6/" + side + "/state");
      const auto controller_topic = declare_parameter<std::string>(
        side + "_controller_topic", side + "_o6_position_controller/commands");

      command_publishers_[side] =
        create_publisher<std_msgs::msg::Float64MultiArray>(controller_topic, 10);
      state_publishers_[side] = create_publisher<sensor_msgs::msg::JointState>(state_topic, 10);
      command_subscriptions_.push_back(create_subscription<sensor_msgs::msg::JointState>(
        command_topic, 10,
        [this, side](const sensor_msgs::msg::JointState::SharedPtr message) {
          publish_controller_command(side, *message);
        }));
    }

    joint_state_subscription_ = create_subscription<sensor_msgs::msg::JointState>(
      joint_states_topic_, 10,
      std::bind(&O6CommandAdapter::publish_legacy_states, this, std::placeholders::_1));
  }

private:
  void publish_controller_command(
    const std::string & side, const sensor_msgs::msg::JointState & message)
  {
    O6Pose raw = o6_open_raw_pose();
    if (message.name.empty()) {
      for (std::size_t index = 0;
        index < kO6JointCount && index < message.position.size(); ++index)
      {
        raw[index] = message.position[index];
      }
    } else {
      const auto & range_names = o6_range_names();
      for (std::size_t message_index = 0;
        message_index < message.name.size() && message_index < message.position.size();
        ++message_index)
      {
        for (std::size_t joint_index = 0; joint_index < kO6JointCount; ++joint_index) {
          if (message.name[message_index] == range_names[joint_index]) {
            raw[joint_index] = message.position[message_index];
          }
        }
      }
    }

    const auto radians = o6_raw_to_radians(raw, side);
    std_msgs::msg::Float64MultiArray output;
    output.data.assign(radians.begin(), radians.end());
    command_publishers_.at(side)->publish(output);
  }

  void publish_legacy_states(const sensor_msgs::msg::JointState::SharedPtr message)
  {
    for (const auto & side : {std::string("left"), std::string("right")}) {
      O6Pose radians{};
      std::array<bool, kO6JointCount> found{};
      const auto names = o6_joint_names(side);
      for (std::size_t message_index = 0;
        message_index < message->name.size() && message_index < message->position.size();
        ++message_index)
      {
        for (std::size_t joint_index = 0; joint_index < kO6JointCount; ++joint_index) {
          if (message->name[message_index] == names[joint_index]) {
            radians[joint_index] = message->position[message_index];
            found[joint_index] = true;
          }
        }
      }
      if (!std::all_of(found.begin(), found.end(), [](bool value) {return value;})) {
        continue;
      }

      const auto raw = o6_radians_to_raw(radians, side);
      sensor_msgs::msg::JointState output;
      output.header = message->header;
      const auto & range_names = o6_range_names();
      output.name.assign(range_names.begin(), range_names.end());
      output.position.assign(raw.begin(), raw.end());
      output.velocity.assign(kO6JointCount, 0.0);
      output.effort.assign(kO6JointCount, 0.0);
      state_publishers_.at(side)->publish(output);
    }
  }

  std::string joint_states_topic_;
  std::unordered_map<std::string, rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr>
    command_publishers_;
  std::unordered_map<std::string, rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr>
    state_publishers_;
  std::vector<rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr>
    command_subscriptions_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_subscription_;
};

}  // namespace openarmx_hand_hardware

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<openarmx_hand_hardware::O6CommandAdapter>());
  rclcpp::shutdown();
  return 0;
}
