#pragma once

#include <array>
#include <string>
#include <vector>

namespace openarmx_linker_o6_c
{

class O6DriverBase
{
public:
  virtual ~O6DriverBase() = default;

  virtual void set_joint_positions(const std::vector<double> & pose) = 0;
  virtual void set_speed(const std::vector<double> & speed) = 0;
  virtual void set_torque(const std::vector<double> & torque) = 0;

  virtual std::vector<int> get_current_status() = 0;
  virtual std::vector<int> get_speed() = 0;
  virtual std::vector<int> get_torque() = 0;
  virtual std::vector<int> get_current() = 0;
  virtual std::vector<int> get_fault() = 0;
  virtual std::vector<int> get_temperature() = 0;
  virtual std::vector<std::vector<float>> get_force() = 0;
  virtual std::string get_matrix_touch_json() = 0;
  virtual std::string get_matrix_touch_mass_json() = 0;
  virtual std::string get_device_info_json() = 0;
};

std::vector<int> six_uint8(const std::vector<double> & values, int default_value);

}  // namespace openarmx_linker_o6_c
