#include "openarmx_linker_o6_c/description/robot_description.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace openarmx_linker_o6_c
{
namespace
{

std::string read_file(const std::string & path)
{
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("Failed to read URDF: " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void replace_all(std::string & text, const std::string & from, const std::string & to)
{
  if (from.empty()) {
    return;
  }
  std::size_t pos = 0;
  while ((pos = text.find(from, pos)) != std::string::npos) {
    text.replace(pos, from.length(), to);
    pos += to.length();
  }
}

std::string body_only(std::string urdf)
{
  const auto robot_tag = urdf.find("<robot");
  const auto start = robot_tag == std::string::npos ? std::string::npos : urdf.find('>', robot_tag);
  const auto end = urdf.rfind("</robot>");
  if (start == std::string::npos || end == std::string::npos || end <= start) {
    return urdf;
  }
  return urdf.substr(start + 1, end - start - 1);
}

std::string prefix_side(std::string urdf, const std::string & package_name, const std::string & side)
{
  const auto side_prefix = side + "_";
  const auto hand_prefix = side == "left" ? std::string("lh_") : std::string("rh_");

  replace_all(urdf, "name=\"" + hand_prefix, "name=\"" + side_prefix + hand_prefix);
  replace_all(urdf, "link=\"" + hand_prefix, "link=\"" + side_prefix + hand_prefix);
  replace_all(urdf, "joint=\"" + hand_prefix, "joint=\"" + side_prefix + hand_prefix);
  replace_all(
    urdf,
    "filename=\"meshes/",
    "filename=\"package://" + package_name + "/urdf/o6/" + side + "/meshes/");
  return urdf;
}

std::string fixed_joint(const std::string & side)
{
  const auto offset = side == "left" ? std::string("0.20 0 0") : std::string("-0.20 0 0");
  const auto rpy = side == "left" ? std::string("0 0 3.14159265359") : std::string("0 0 0");
  const auto base_link = side == "left" ? std::string("left_lh_hand_base_link") :
    std::string("right_rh_hand_base_link");

  std::ostringstream ss;
  ss << "  <joint name=\"world_to_" << side << "_hand\" type=\"fixed\">\n"
     << "    <origin xyz=\"" << offset << "\" rpy=\"" << rpy << "\" />\n"
     << "    <parent link=\"world\" />\n"
     << "    <child link=\"" << base_link << "\" />\n"
     << "  </joint>\n";
  return ss.str();
}

}  // namespace

std::string build_robot_description(const std::string & package_share, const std::string & hand)
{
  std::vector<std::string> sides;
  if (hand == "left") {
    sides.push_back("left");
  } else if (hand == "right") {
    sides.push_back("right");
  } else {
    sides.push_back("left");
    sides.push_back("right");
  }

  std::ostringstream robot;
  robot << "<?xml version=\"1.0\"?>\n"
        << "<robot name=\"hands_description\">\n"
        << "  <link name=\"world\" />\n";

  bool material_added = false;
  for (const auto & side : sides) {
    const auto path = package_share + "/urdf/o6/" + side + "/linkerhand_o6_" + side + ".urdf";
    auto urdf = prefix_side(body_only(read_file(path)), "hands_description", side);

    if (material_added) {
      const auto first_material = urdf.find("<material ");
      const auto first_link = urdf.find("<link");
      if (first_material != std::string::npos && first_link != std::string::npos && first_material < first_link) {
        urdf.erase(first_material, first_link - first_material);
      }
    }
    material_added = true;

    robot << urdf << "\n" << fixed_joint(side);
  }

  robot << "</robot>\n";
  return robot.str();
}

}  // namespace openarmx_linker_o6_c
