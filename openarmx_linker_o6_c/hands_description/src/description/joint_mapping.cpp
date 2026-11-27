#include "openarmx_linker_o6_c/description/joint_mapping.hpp"

#include <algorithm>

namespace openarmx_linker_o6_c
{
namespace
{
const O6Pose kO6LeftMin{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
const O6Pose kO6LeftMax{0.58, 1.36, 1.6, 1.6, 1.6, 1.6};
const std::array<int, kO6JointCount> kO6LeftDirect{-1, -1, -1, -1, -1, -1};

const O6Pose kO6RightMin{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
const O6Pose kO6RightMax{0.58, 1.36, 1.6, 1.6, 1.6, 1.6};
const std::array<int, kO6JointCount> kO6RightDirect{-1, -1, -1, -1, -1, -1};

const std::array<std::string, kO6JointCount> kRangeNames{
  "thumb_cmc_pitch",
  "thumb_cmc_yaw",
  "index_mcp_pitch",
  "middle_mcp_pitch",
  "ring_mcp_pitch",
  "pinky_mcp_pitch"};

const O6Pose kOpenPose{200.0, 255.0, 255.0, 255.0, 255.0, 255.0};
}  // namespace

const std::array<std::string, kO6JointCount> & range_names()
{
  return kRangeNames;
}

const O6Pose & open_pose()
{
  return kOpenPose;
}

double clamp(double value, double low, double high)
{
  return std::max(low, std::min(high, value));
}

double scale_value(double value, double from_low, double from_high, double to_low, double to_high)
{
  if (from_high == from_low) {
    return to_low;
  }
  return (value - from_low) * (to_high - to_low) / (from_high - from_low) + to_low;
}

O6Pose normalize_range(const std::vector<double> & values)
{
  O6Pose pose = kOpenPose;
  const auto count = std::min(values.size(), pose.size());
  for (std::size_t i = 0; i < count; ++i) {
    pose[i] = clamp(values[i], 0.0, 255.0);
  }
  return pose;
}

O6Pose normalize_range(const std::vector<float> & values)
{
  std::vector<double> converted;
  converted.reserve(values.size());
  for (const auto value : values) {
    converted.push_back(static_cast<double>(value));
  }
  return normalize_range(converted);
}

O6Pose range_to_arc(const O6Pose & values, const std::string & side)
{
  const auto & mins = side == "left" ? kO6LeftMin : kO6RightMin;
  const auto & maxs = side == "left" ? kO6LeftMax : kO6RightMax;
  const auto & directs = side == "left" ? kO6LeftDirect : kO6RightDirect;

  O6Pose arcs{};
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto value = clamp(values[i], 0.0, 255.0);
    if (directs[i] == -1) {
      arcs[i] = scale_value(value, 0.0, 255.0, maxs[i], mins[i]);
    } else {
      arcs[i] = scale_value(value, 0.0, 255.0, mins[i], maxs[i]);
    }
  }
  return arcs;
}

std::unordered_map<std::string, double> o6_urdf_joint_positions(
  const O6Pose & values,
  const std::string & side)
{
  const auto arcs = range_to_arc(values, side);
  const auto model_prefix = side == "left" ? std::string("lh") : std::string("rh");
  const auto joint_prefix = side + "_" + model_prefix + "_";

  return {
    {joint_prefix + "thumb_cmc_pitch", arcs[0]},
    {joint_prefix + "thumb_cmc_yaw", arcs[1]},
    {joint_prefix + "index_mcp_pitch", arcs[2]},
    {joint_prefix + "middle_mcp_pitch", arcs[3]},
    {joint_prefix + "ring_mcp_pitch", arcs[4]},
    {joint_prefix + "pinky_mcp_pitch", arcs[5]},
  };
}

std::vector<std::string> prefixed_o6_joint_names(const std::string & side)
{
  const auto model_prefix = side == "left" ? std::string("lh") : std::string("rh");
  const auto prefix = side + "_" + model_prefix + "_";
  std::vector<std::string> names;
  names.reserve(kRangeNames.size());
  for (const auto & name : kRangeNames) {
    names.push_back(prefix + name);
  }
  return names;
}

}  // namespace openarmx_linker_o6_c
