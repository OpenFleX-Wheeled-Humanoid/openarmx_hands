#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/int32_multi_array.hpp"

namespace
{

using O6Pose = std::array<double, 6>;

constexpr O6Pose kO6OpenPose{200.0, 255.0, 255.0, 255.0, 255.0, 255.0};
const std::vector<std::string> kO6Names{
  "thumb_cmc_pitch",
  "thumb_cmc_yaw",
  "index_mcp_pitch",
  "middle_mcp_pitch",
  "ring_mcp_pitch",
  "pinky_mcp_pitch",
};

double clamp(double value, double low, double high)
{
  return std::max(low, std::min(high, value));
}

double scale(double value, double src_low, double src_high, double dst_low, double dst_high)
{
  if (src_high == src_low) {
    return dst_low;
  }
  return (value - src_low) * (dst_high - dst_low) / (src_high - src_low) + dst_low;
}

std::vector<double> normalized_higvr_values(const std::vector<int32_t> & data)
{
  std::vector<double> values;
  values.reserve(6);
  for (std::size_t i = 0; i < 6; ++i) {
    const auto value = i < data.size() ? static_cast<double>(data[i]) : 100.0;
    values.push_back(clamp(value, 0.0, 100.0));
  }
  return values;
}

}  // namespace

class HigvrToO6Node : public rclcpp::Node
{
public:
  HigvrToO6Node()
  : Node("higvr_to_o6_node")
  {
    hand_ = declare_parameter<std::string>("hand", "both");
    left_input_topic_ = declare_parameter<std::string>("left_input_topic", "/higvr/left_hand");
    right_input_topic_ = declare_parameter<std::string>("right_input_topic", "/higvr/right_hand");
    left_output_topic_ = declare_parameter<std::string>("left_output_topic", "/openarmx/o6/left/command");
    right_output_topic_ = declare_parameter<std::string>("right_output_topic", "/openarmx/o6/right/command");
    publish_rate_ = declare_parameter<double>("publish_rate", 100.0);
    thumb_close_value_ = declare_parameter<double>("thumb_close_value", 0.0);
    finger_close_value_ = declare_parameter<double>("finger_close_value", 0.0);
    thumb_yaw_min_ = declare_parameter<double>("thumb_yaw_min", 0.0);
    thumb_yaw_max_ = declare_parameter<double>("thumb_yaw_max", 255.0);
    pinky_open_value_ = declare_parameter<double>("pinky_open_value", 255.0);
    const auto enable_left = declare_parameter<bool>("enable_left", true) &&
      (hand_ == "left" || hand_ == "both");
    const auto enable_right = declare_parameter<bool>("enable_right", true) &&
      (hand_ == "right" || hand_ == "both");

    if (enable_left) {
      sides_.push_back("left");
      latest_["left"] = {};
      pubs_["left"] = create_publisher<sensor_msgs::msg::JointState>(left_output_topic_, 10);
      left_sub_ = create_subscription<std_msgs::msg::Int32MultiArray>(
        left_input_topic_, 10,
        [this](const std_msgs::msg::Int32MultiArray::SharedPtr msg) {
          update("left", msg);
        });
      RCLCPP_INFO(
        get_logger(), "左手 HIGVR -> O6: %s -> %s",
        left_input_topic_.c_str(), left_output_topic_.c_str());
    }

    if (enable_right) {
      sides_.push_back("right");
      latest_["right"] = {};
      pubs_["right"] = create_publisher<sensor_msgs::msg::JointState>(right_output_topic_, 10);
      right_sub_ = create_subscription<std_msgs::msg::Int32MultiArray>(
        right_input_topic_, 10,
        [this](const std_msgs::msg::Int32MultiArray::SharedPtr msg) {
          update("right", msg);
        });
      RCLCPP_INFO(
        get_logger(), "右手 HIGVR -> O6: %s -> %s",
        right_input_topic_.c_str(), right_output_topic_.c_str());
    }

    timer_ = create_wall_timer(
      std::chrono::duration<double>(1.0 / std::max(publish_rate_, 1.0)),
      std::bind(&HigvrToO6Node::publish_commands, this));
  }

private:
  void update(const std::string & side, const std_msgs::msg::Int32MultiArray::SharedPtr msg)
  {
    latest_[side] = normalized_higvr_values(msg->data);
  }

  O6Pose higvr_to_o6(const std::vector<double> & values) const
  {
    O6Pose pose = kO6OpenPose;
    const auto thumb_yaw = values[5];

    pose[0] = scale(values[0], 0.0, 100.0, thumb_close_value_, 200.0);
    pose[1] = scale(thumb_yaw, 0.0, 100.0, thumb_yaw_min_, thumb_yaw_max_);
    pose[2] = scale(values[1], 0.0, 100.0, finger_close_value_, 255.0);
    pose[3] = scale(values[2], 0.0, 100.0, finger_close_value_, 255.0);
    pose[4] = scale(values[3], 0.0, 100.0, finger_close_value_, 255.0);
    pose[5] = scale(values[4], 0.0, 100.0, finger_close_value_, pinky_open_value_);

    for (auto & value : pose) {
      value = clamp(value, 0.0, 255.0);
    }
    return pose;
  }

  void publish_commands()
  {
    for (const auto & side : sides_) {
      const auto found = latest_.find(side);
      if (found == latest_.end() || found->second.empty()) {
        continue;
      }

      const auto pose = higvr_to_o6(found->second);
      sensor_msgs::msg::JointState msg;
      msg.header.stamp = now();
      msg.name = kO6Names;
      msg.position.assign(pose.begin(), pose.end());
      msg.velocity.clear();
      msg.effort.clear();
      pubs_.at(side)->publish(msg);
    }
  }

  std::string hand_;
  std::string left_input_topic_;
  std::string right_input_topic_;
  std::string left_output_topic_;
  std::string right_output_topic_;
  double publish_rate_{100.0};
  double thumb_close_value_{0.0};
  double finger_close_value_{0.0};
  double thumb_yaw_min_{255.0};
  double thumb_yaw_max_{0.0};
  double pinky_open_value_{255.0};

  std::vector<std::string> sides_;
  std::unordered_map<std::string, std::vector<double>> latest_;
  std::unordered_map<std::string, rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr> pubs_;
  rclcpp::Subscription<std_msgs::msg::Int32MultiArray>::SharedPtr left_sub_;
  rclcpp::Subscription<std_msgs::msg::Int32MultiArray>::SharedPtr right_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HigvrToO6Node>());
  rclcpp::shutdown();
  return 0;
}
