#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"

#include "openarmx_linker_o6_c/description/robot_description.hpp"

class O6RobotDescriptionNode : public rclcpp::Node
{
public:
  O6RobotDescriptionNode()
  : Node("openarmx_o6_c_robot_description")
  {
    const auto package_share = declare_parameter<std::string>("package_share", "");
    const auto hand = declare_parameter<std::string>("hand", "both");
    if (package_share.empty()) {
      throw std::runtime_error("package_share parameter is required");
    }
    const auto description = openarmx_linker_o6_c::build_robot_description(package_share, hand);
    set_parameter(rclcpp::Parameter("robot_description", description));
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<O6RobotDescriptionNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
