#include "openarmx_hand_hardware/o6_joint_mapping.hpp"

#include <algorithm>

namespace openarmx_hand_hardware
{
namespace
{

const O6Pose kJointMinimum{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
const O6Pose kJointMaximum{0.58, 1.36, 1.60, 1.60, 1.60, 1.60};
const O6Pose kOpenRawPose{200.0, 255.0, 255.0, 255.0, 255.0, 255.0};
const std::array<std::string, kO6JointCount> kRangeNames{
  "thumb_cmc_pitch",
  "thumb_cmc_yaw",
  "index_mcp_pitch",
  "middle_mcp_pitch",
  "ring_mcp_pitch",
  "pinky_mcp_pitch",
};

double scale(double value, double source_min, double source_max, double target_min, double target_max)
{
  if (source_max == source_min) {
    return target_min;
  }
  return (value - source_min) * (target_max - target_min) /
         (source_max - source_min) + target_min;
}

}  // namespace

const std::array<std::string, kO6JointCount> & o6_range_names()
{
  return kRangeNames;
}

std::vector<std::string> o6_joint_names(const std::string & side)
{
  const auto model_prefix = side == "left" ? std::string("lh") : std::string("rh");
  const auto prefix = side + "_" + model_prefix + "_";
  std::vector<std::string> names;
  names.reserve(kO6JointCount);
  for (const auto & name : kRangeNames) {
    names.push_back(prefix + name);
  }
  return names;
}

O6Pose o6_open_raw_pose()
{
  return kOpenRawPose;
}

O6Pose o6_raw_to_radians(const O6Pose & raw, const std::string & /*side*/)
{
  O6Pose radians{};
  for (std::size_t index = 0; index < kO6JointCount; ++index) {
    const auto value = std::clamp(raw[index], 0.0, 255.0);
    radians[index] = scale(value, 0.0, 255.0, kJointMaximum[index], kJointMinimum[index]);
  }
  return radians;
}

O6Pose o6_radians_to_raw(const O6Pose & radians, const std::string & /*side*/)
{
  O6Pose raw{};
  for (std::size_t index = 0; index < kO6JointCount; ++index) {
    const auto value = std::clamp(
      radians[index], kJointMinimum[index], kJointMaximum[index]);
    raw[index] = scale(value, kJointMinimum[index], kJointMaximum[index], 255.0, 0.0);
  }
  return raw;
}

}  // namespace openarmx_hand_hardware
