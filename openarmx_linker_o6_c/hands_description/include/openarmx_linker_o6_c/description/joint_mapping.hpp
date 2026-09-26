#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace openarmx_linker_o6_c
{

constexpr std::size_t kO6JointCount = 6;

using O6Pose = std::array<double, kO6JointCount>;

const std::array<std::string, kO6JointCount> & range_names();
const O6Pose & open_pose();

double clamp(double value, double low, double high);
double scale_value(double value, double from_low, double from_high, double to_low, double to_high);
O6Pose normalize_range(const std::vector<double> & values);
O6Pose normalize_range(const std::vector<float> & values);
O6Pose range_to_arc(const O6Pose & values, const std::string & side);
std::unordered_map<std::string, double> o6_urdf_joint_positions(
  const O6Pose & values,
  const std::string & side);
std::vector<std::string> prefixed_o6_joint_names(const std::string & side);

}  // namespace openarmx_linker_o6_c
