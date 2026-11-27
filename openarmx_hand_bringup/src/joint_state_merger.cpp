#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

class JointStateMerger : public rclcpp::Node
{
public:
  JointStateMerger()
  : Node("openarmx_hand_joint_state_merger")
  {
    const auto arm_topic = declare_parameter<std::string>("arm_topic", "joint_states");
    const auto hand_topic = declare_parameter<std::string>(
      "hand_topic", "openarmx/o6/joint_states");
    const auto output_topic = declare_parameter<std::string>(
      "output_topic", "openarmx/display/joint_states");
    const auto publish_rate = declare_parameter<double>("publish_rate", 50.0);

    arm_sub_ = create_subscription<sensor_msgs::msg::JointState>(
      arm_topic, 10,
      [this](sensor_msgs::msg::JointState::SharedPtr message) {arm_state_ = message;});
    hand_sub_ = create_subscription<sensor_msgs::msg::JointState>(
      hand_topic, 10,
      [this](sensor_msgs::msg::JointState::SharedPtr message) {hand_state_ = message;});
    publisher_ = create_publisher<sensor_msgs::msg::JointState>(output_topic, 10);
    timer_ = create_wall_timer(
      std::chrono::duration<double>(1.0 / std::max(publish_rate, 1.0)),
      std::bind(&JointStateMerger::publish, this));
  }

private:
  struct JointValue
  {
    double position{0.0};
    double velocity{0.0};
    double effort{0.0};
  };

  static void merge(
    const sensor_msgs::msg::JointState & message,
    std::vector<std::string> & names,
    std::unordered_map<std::string, JointValue> & values)
  {
    for (std::size_t index = 0; index < message.name.size(); ++index) {
      const auto & name = message.name[index];
      if (name.empty()) {
        continue;
      }
      if (values.find(name) == values.end()) {
        names.push_back(name);
      }
      auto & value = values[name];
      if (index < message.position.size()) {
        value.position = message.position[index];
      }
      if (index < message.velocity.size()) {
        value.velocity = message.velocity[index];
      }
      if (index < message.effort.size()) {
        value.effort = message.effort[index];
      }
    }
  }

  void publish()
  {
    if (!arm_state_) {
      return;
    }
    std::vector<std::string> names;
    std::unordered_map<std::string, JointValue> values;
    merge(*arm_state_, names, values);
    if (hand_state_) {
      merge(*hand_state_, names, values);
    }

    sensor_msgs::msg::JointState output;
    output.header.stamp = now();
    output.name = names;
    for (const auto & name : names) {
      const auto & value = values.at(name);
      output.position.push_back(value.position);
      output.velocity.push_back(value.velocity);
      output.effort.push_back(value.effort);
    }
    publisher_->publish(output);
  }

  sensor_msgs::msg::JointState::SharedPtr arm_state_;
  sensor_msgs::msg::JointState::SharedPtr hand_state_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr arm_sub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr hand_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<JointStateMerger>());
  rclcpp::shutdown();
  return 0;
}
