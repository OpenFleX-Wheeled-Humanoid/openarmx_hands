#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include "openarmx_linker_o6_c/description/joint_mapping.hpp"

class O6JointStateMapperNode : public rclcpp::Node
{
public:
  O6JointStateMapperNode()
  : Node("openarmx_o6_c_joint_state_mapper")
  {
    const auto hand = declare_parameter<std::string>("hand", "both");
    left_input_topic_ = declare_parameter<std::string>("left_input_topic", "/openarmx/o6/left/command");
    right_input_topic_ = declare_parameter<std::string>("right_input_topic", "/openarmx/o6/right/command");
    joint_states_topic_ = declare_parameter<std::string>("joint_states_topic", "/joint_states");
    const auto publish_rate = declare_parameter<double>("publish_rate", 50.0);

    if (hand == "left") {
      sides_ = {"left"};
    } else if (hand == "right") {
      sides_ = {"right"};
    } else {
      sides_ = {"left", "right"};
    }

    left_pose_ = openarmx_linker_o6_c::open_pose();
    right_pose_ = openarmx_linker_o6_c::open_pose();

    if (has_side("left")) {
      left_sub_ = create_subscription<sensor_msgs::msg::JointState>(
        left_input_topic_, 10,
        [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
          left_pose_ = openarmx_linker_o6_c::normalize_range(msg->position);
        });
      RCLCPP_INFO(get_logger(), "订阅左手 O6 输入: %s", left_input_topic_.c_str());
    }
    if (has_side("right")) {
      right_sub_ = create_subscription<sensor_msgs::msg::JointState>(
        right_input_topic_, 10,
        [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
          right_pose_ = openarmx_linker_o6_c::normalize_range(msg->position);
        });
      RCLCPP_INFO(get_logger(), "订阅右手 O6 输入: %s", right_input_topic_.c_str());
    }

    joint_pub_ = create_publisher<sensor_msgs::msg::JointState>(joint_states_topic_, 10);
    timer_ = create_wall_timer(
      std::chrono::duration<double>(1.0 / std::max(publish_rate, 1.0)),
      std::bind(&O6JointStateMapperNode::publish_joint_states, this));
  }

private:
  bool has_side(const std::string & side) const
  {
    return std::find(sides_.begin(), sides_.end(), side) != sides_.end();
  }

  void append_side(
    const std::string & side,
    const openarmx_linker_o6_c::O6Pose & pose,
    std::vector<std::string> & names,
    std::vector<double> & positions)
  {
    const auto side_names = openarmx_linker_o6_c::prefixed_o6_joint_names(side);
    const auto side_positions = openarmx_linker_o6_c::o6_urdf_joint_positions(pose, side);
    for (const auto & name : side_names) {
      names.push_back(name);
      const auto it = side_positions.find(name);
      positions.push_back(it == side_positions.end() ? 0.0 : it->second);
    }
  }

  void publish_joint_states()
  {
    sensor_msgs::msg::JointState msg;
    msg.header.stamp = now();
    if (has_side("left")) {
      append_side("left", left_pose_, msg.name, msg.position);
    }
    if (has_side("right")) {
      append_side("right", right_pose_, msg.name, msg.position);
    }
    msg.velocity.assign(msg.position.size(), 0.0);
    msg.effort.assign(msg.position.size(), 0.0);
    joint_pub_->publish(msg);
  }

  std::vector<std::string> sides_;
  std::string left_input_topic_;
  std::string right_input_topic_;
  std::string joint_states_topic_;
  openarmx_linker_o6_c::O6Pose left_pose_{};
  openarmx_linker_o6_c::O6Pose right_pose_{};
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr left_sub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr right_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<O6JointStateMapperNode>());
  rclcpp::shutdown();
  return 0;
}
