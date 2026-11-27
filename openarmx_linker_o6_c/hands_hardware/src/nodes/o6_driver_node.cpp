#include <chrono>
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/msg/point_field.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"
#include "std_msgs/msg/int32_multi_array.hpp"
#include "std_msgs/msg/string.hpp"

#include "openarmx_linker_o6_c/description/joint_mapping.hpp"
#include "openarmx_linker_o6_c/drivers/can_driver.hpp"
#include "openarmx_linker_o6_c/drivers/o6_driver_base.hpp"
#include "openarmx_linker_o6_c/drivers/rs485_driver.hpp"

class O6DriverNode : public rclcpp::Node
{
public:
  O6DriverNode()
  : Node("openarmx_o6_c_driver")
  {
    side_ = declare_parameter<std::string>("side", "right");
    can_name_ = declare_parameter<std::string>("can", "can0");
    can_id_ = declare_parameter<int>("can_id", 0x27);
    transport_ = declare_parameter<std::string>("transport", "can");
    modbus_port_ = declare_parameter<std::string>("modbus_port", "/dev/ttyUSB0");
    modbus_id_ = declare_parameter<int>("modbus_id", 0x27);
    modbus_baudrate_ = declare_parameter<int>("modbus_baudrate", 115200);
    publish_rate_ = declare_parameter<double>("publish_rate", 100.0);
    poll_touch_ = declare_parameter<bool>("poll_touch", false);
    poll_matrix_touch_ = declare_parameter<bool>("poll_matrix_touch", false);
    poll_diagnostics_ = declare_parameter<bool>("poll_diagnostics", true);
    poll_device_info_ = declare_parameter<bool>("poll_device_info", true);
    info_rate_ = declare_parameter<double>("info_rate", 1.0);
    const auto init_speed = declare_parameter<std::vector<double>>(
      "init_speed", std::vector<double>{255, 255, 255, 255, 255, 255});
    const auto init_torque = declare_parameter<std::vector<double>>(
      "init_torque", std::vector<double>{255, 255, 255, 255, 255, 255});

    if (transport_ == "rs485") {
      hand_ = std::make_unique<openarmx_linker_o6_c::O6Rs485Driver>(
        modbus_port_, modbus_id_, modbus_baudrate_);
    } else {
      transport_ = "can";
      hand_ = std::make_unique<openarmx_linker_o6_c::O6CanDriver>(can_name_, can_id_);
    }

    last_state_ = openarmx_linker_o6_c::open_pose();
    try {
      hand_->set_speed(init_speed);
      hand_->set_torque(init_torque);
      const auto & open = openarmx_linker_o6_c::open_pose();
      hand_->set_joint_positions(std::vector<double>(open.begin(), open.end()));
    } catch (const std::exception & exc) {
      RCLCPP_WARN(get_logger(), "O6 初始化命令发送失败: %s", exc.what());
    }

    const auto base = "/openarmx/o6/" + side_;
    command_sub_ = create_subscription<sensor_msgs::msg::JointState>(
      base + "/command", 10,
      std::bind(&O6DriverNode::command_callback, this, std::placeholders::_1));
    setting_sub_ = create_subscription<std_msgs::msg::String>(
      base + "/setting_cmd", 10,
      std::bind(&O6DriverNode::setting_callback, this, std::placeholders::_1));
    state_pub_ = create_publisher<sensor_msgs::msg::JointState>(base + "/state", 10);
    info_pub_ = create_publisher<std_msgs::msg::String>(base + "/info", 10);
    force_pub_ = create_publisher<std_msgs::msg::Float32MultiArray>(base + "/force", 10);
    matrix_pub_ = create_publisher<std_msgs::msg::String>(base + "/matrix_touch", 10);
    matrix_mass_pub_ = create_publisher<std_msgs::msg::String>(base + "/matrix_touch_mass", 10);
    matrix_pc_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>(base + "/matrix_touch_pc", 10);
    temperature_pub_ = create_publisher<std_msgs::msg::Float32MultiArray>(base + "/temperature", 10);
    current_pub_ = create_publisher<std_msgs::msg::Float32MultiArray>(base + "/current", 10);
    fault_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>(base + "/fault", 10);
    device_info_pub_ = create_publisher<std_msgs::msg::String>(base + "/device_info", 10);

    timer_ = create_wall_timer(
      std::chrono::duration<double>(1.0 / std::max(publish_rate_, 1.0)),
      std::bind(&O6DriverNode::poll_and_publish, this));

    RCLCPP_INFO(
      get_logger(), "O6 %s C++ driver started: transport=%s, can=%s, can_id=0x%x",
      side_.c_str(), transport_.c_str(), can_name_.c_str(), can_id_);
  }

private:
  static std::string vector_json(const std::vector<int> & values)
  {
    std::ostringstream ss;
    ss << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (i > 0) {
        ss << ",";
      }
      ss << values[i];
    }
    ss << "]";
    return ss.str();
  }

  static std::vector<double> extract_array(const std::string & text, const std::string & key)
  {
    const auto key_pos = text.find("\"" + key + "\"");
    if (key_pos == std::string::npos) {
      return {};
    }
    const auto start = text.find("[", key_pos);
    const auto end = text.find("]", start);
    if (start == std::string::npos || end == std::string::npos || end <= start) {
      return {};
    }
    std::vector<double> values;
    std::stringstream ss(text.substr(start + 1, end - start - 1));
    std::string item;
    while (std::getline(ss, item, ',')) {
      try {
        values.push_back(std::stod(item));
      } catch (...) {
      }
    }
    return values;
  }

  static std::vector<unsigned char> extract_matrix_values(const std::string & json)
  {
    std::vector<unsigned char> values;
    values.reserve(360);
    bool in_number = false;
    bool has_digit = false;
    bool negative = false;
    bool in_string = false;
    bool escape = false;
    int current = 0;
    for (const auto ch : json) {
      if (in_string) {
        if (escape) {
          escape = false;
        } else if (ch == '\\') {
          escape = true;
        } else if (ch == '"') {
          in_string = false;
        }
        continue;
      }
      if (ch == '"') {
        in_string = true;
        continue;
      }
      if (ch == '-') {
        in_number = true;
        negative = true;
        has_digit = false;
        current = 0;
      } else if (ch >= '0' && ch <= '9') {
        if (!in_number) {
          in_number = true;
          negative = false;
          current = 0;
        }
        has_digit = true;
        current = current * 10 + (ch - '0');
      } else if (in_number) {
        if (has_digit) {
          const auto signed_value = negative ? -current : current;
          const auto clipped = std::clamp(signed_value, 0, 255);
          values.push_back(static_cast<unsigned char>(clipped));
        }
        in_number = false;
        has_digit = false;
        negative = false;
        current = 0;
      }
    }
    if (in_number && has_digit) {
      const auto signed_value = negative ? -current : current;
      const auto clipped = std::clamp(signed_value, 0, 255);
      values.push_back(static_cast<unsigned char>(clipped));
    }
    return values;
  }

  void command_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
  {
    const auto pose = openarmx_linker_o6_c::normalize_range(msg->position);
    try {
      hand_->set_joint_positions(std::vector<double>(pose.begin(), pose.end()));
    } catch (const std::exception & exc) {
      RCLCPP_ERROR(get_logger(), "发送 O6 位置命令失败: %s", exc.what());
    }
  }

  void setting_callback(const std_msgs::msg::String::SharedPtr msg)
  {
    try {
      if (msg->data.find("set_speed") != std::string::npos) {
        const auto speed = extract_array(msg->data, "speed");
        hand_->set_speed(speed.empty() ? std::vector<double>{255, 255, 255, 255, 255, 255} : speed);
      } else if (msg->data.find("set_torque") != std::string::npos) {
        const auto torque = extract_array(msg->data, "torque");
        hand_->set_torque(torque.empty() ? std::vector<double>{255, 255, 255, 255, 255, 255} : torque);
      } else {
        RCLCPP_WARN(get_logger(), "未知设置命令: %s", msg->data.c_str());
      }
    } catch (const std::exception & exc) {
      RCLCPP_ERROR(get_logger(), "处理设置命令失败: %s", exc.what());
    }
  }

  void poll_and_publish()
  {
    std::vector<int> state;
    try {
      state = hand_->get_current_status();
      if (state.size() == openarmx_linker_o6_c::kO6JointCount) {
        for (std::size_t i = 0; i < state.size(); ++i) {
          last_state_[i] = static_cast<double>(state[i]);
        }
      }
    } catch (const std::exception & exc) {
      RCLCPP_WARN(get_logger(), "读取 O6 状态失败: %s", exc.what());
      state.assign(last_state_.begin(), last_state_.end());
    }

    sensor_msgs::msg::JointState msg;
    msg.header.stamp = now();
    const auto & names = openarmx_linker_o6_c::range_names();
    msg.name.assign(names.begin(), names.end());
    const auto pose = openarmx_linker_o6_c::normalize_range(
      std::vector<double>(state.begin(), state.end()));
    msg.position.assign(pose.begin(), pose.end());
    msg.velocity.assign(msg.position.size(), 0.0);
    msg.effort.assign(msg.position.size(), 0.0);
    state_pub_->publish(msg);

    const auto now_time = get_clock()->now();
    if ((now_time - last_info_time_).seconds() >= 1.0 / std::max(info_rate_, 0.1)) {
      if (poll_diagnostics_) {
        publish_diagnostics();
      }
      if (poll_device_info_) {
        publish_device_info();
      }
      publish_info();
      last_info_time_ = now_time;
    }

    if (poll_touch_) {
      publish_force();
    }
    if (poll_matrix_touch_) {
      publish_matrix_touch();
    }
  }

  void publish_info()
  {
    try {
      std_msgs::msg::String msg;
      std::ostringstream ss;
      ss << "{"
         << "\"side\":\"" << side_ << "\","
         << "\"hand_joint\":\"O6\","
         << "\"transport\":\"" << transport_ << "\","
         << "\"can\":\"" << can_name_ << "\","
         << "\"can_id\":" << can_id_ << ","
         << "\"modbus_port\":\"" << modbus_port_ << "\","
         << "\"modbus_id\":" << modbus_id_ << ","
         << "\"speed\":" << vector_json(hand_->get_speed()) << ","
         << "\"torque\":" << vector_json(hand_->get_torque()) << ","
         << "\"current\":" << vector_json(hand_->get_current()) << ","
         << "\"fault\":" << vector_json(hand_->get_fault()) << ","
         << "\"temperature\":" << vector_json(hand_->get_temperature()) << ","
         << "\"device_info\":" << last_device_info_json_
         << "}";
      msg.data = ss.str();
      info_pub_->publish(msg);
    } catch (const std::exception & exc) {
      RCLCPP_WARN(get_logger(), "发布 O6 信息失败: %s", exc.what());
    }
  }

  void publish_force()
  {
    try {
      const auto force_groups = hand_->get_force();
      std_msgs::msg::Float32MultiArray msg;
      for (const auto & group : force_groups) {
        msg.data.insert(msg.data.end(), group.begin(), group.end());
      }
      force_pub_->publish(msg);
    } catch (const std::exception & exc) {
      RCLCPP_WARN(get_logger(), "发布 O6 压感失败: %s", exc.what());
    }
  }

  void publish_diagnostics()
  {
    try {
      const auto temperature = hand_->get_temperature();
      const auto current = hand_->get_current();
      const auto fault = hand_->get_fault();

      std_msgs::msg::Float32MultiArray temp_msg;
      for (const auto value : temperature) {
        temp_msg.data.push_back(static_cast<float>(value));
      }
      temperature_pub_->publish(temp_msg);

      std_msgs::msg::Float32MultiArray current_msg;
      for (const auto value : current) {
        current_msg.data.push_back(static_cast<float>(value));
      }
      current_pub_->publish(current_msg);

      std_msgs::msg::Int32MultiArray fault_msg;
      fault_msg.data.assign(fault.begin(), fault.end());
      fault_pub_->publish(fault_msg);
    } catch (const std::exception & exc) {
      RCLCPP_WARN(get_logger(), "发布 O6 诊断状态失败: %s", exc.what());
    }
  }

  void publish_device_info()
  {
    try {
      std_msgs::msg::String msg;
      const auto device_info = hand_->get_device_info_json();
      last_device_info_json_ = device_info.empty() ? "{}" : device_info;
      msg.data = "{\"side\":\"" + side_ + "\"," + last_device_info_json_.substr(1);
      device_info_pub_->publish(msg);
    } catch (const std::exception & exc) {
      RCLCPP_WARN(get_logger(), "发布 O6 设备信息失败: %s", exc.what());
    }
  }

  void publish_matrix_touch()
  {
    std_msgs::msg::String msg;
    msg.data = hand_->get_matrix_touch_json();
    matrix_pub_->publish(msg);

    std_msgs::msg::String mass_msg;
    mass_msg.data = hand_->get_matrix_touch_mass_json();
    matrix_mass_pub_->publish(mass_msg);
    publish_matrix_point_cloud(msg.data);
  }

  void publish_matrix_point_cloud(const std::string & matrix_json)
  {
    auto values = extract_matrix_values(matrix_json);
    if (values.size() > 360) {
      values.erase(values.begin(), values.end() - 360);
    }

    sensor_msgs::msg::PointCloud2 msg;
    msg.header.stamp = now();
    msg.header.frame_id = "openarmx_o6_" + side_ + "_pressure";
    msg.height = 1;
    msg.width = static_cast<std::uint32_t>(values.size());

    sensor_msgs::msg::PointField field;
    field.name = "val";
    field.offset = 0;
    field.datatype = sensor_msgs::msg::PointField::UINT8;
    field.count = 1;
    msg.fields.push_back(field);

    msg.is_bigendian = false;
    msg.point_step = 1;
    msg.row_step = msg.point_step * msg.width;
    msg.data = values;
    msg.is_dense = true;
    matrix_pc_pub_->publish(msg);
  }

  std::string side_;
  std::string can_name_;
  int can_id_{0};
  std::string transport_;
  std::string modbus_port_;
  int modbus_id_{0};
  int modbus_baudrate_{115200};
  double publish_rate_{100.0};
  bool poll_touch_{false};
  bool poll_matrix_touch_{false};
  bool poll_diagnostics_{true};
  bool poll_device_info_{true};
  double info_rate_{1.0};
  openarmx_linker_o6_c::O6Pose last_state_{};
  rclcpp::Time last_info_time_{0, 0, RCL_ROS_TIME};
  std::string last_device_info_json_{"{}"};
  std::unique_ptr<openarmx_linker_o6_c::O6DriverBase> hand_;

  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr command_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr setting_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr state_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr info_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr force_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr matrix_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr matrix_mass_pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr matrix_pc_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr temperature_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr current_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr fault_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr device_info_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<O6DriverNode>());
  rclcpp::shutdown();
  return 0;
}
