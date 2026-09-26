#include <iostream>
#include <stdexcept>
#include <string>

#include "openarmx_linker_o6_c/description/robot_description.hpp"

int main(int argc, char ** argv)
{
  if (argc < 2) {
    std::cerr << "usage: o6_robot_description_cli <package_share> [left|right|both]\n";
    return 2;
  }
  const std::string package_share = argv[1];
  const std::string hand = argc > 2 ? argv[2] : "both";
  try {
    std::cout << openarmx_linker_o6_c::build_robot_description(package_share, hand);
  } catch (const std::exception & exc) {
    std::cerr << exc.what() << "\n";
    return 1;
  }
  return 0;
}
