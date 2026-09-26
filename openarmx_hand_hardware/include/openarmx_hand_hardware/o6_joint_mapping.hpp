#pragma once

#include <array>
#include <string>
#include <vector>

namespace openarmx_hand_hardware
{

constexpr std::size_t kO6JointCount = 6;
using O6Pose = std::array<double, kO6JointCount>;

const std::array<std::string, kO6JointCount> & o6_range_names();
std::vector<std::string> o6_joint_names(const std::string & side);
O6Pose o6_open_raw_pose();
O6Pose o6_raw_to_radians(const O6Pose & raw, const std::string & side);
O6Pose o6_radians_to_raw(const O6Pose & radians, const std::string & side);

}  // namespace openarmx_hand_hardware
