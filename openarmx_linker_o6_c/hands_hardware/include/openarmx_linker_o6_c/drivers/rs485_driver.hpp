#pragma once

#include <string>
#include <vector>

#include "openarmx_linker_o6_c/drivers/o6_driver_base.hpp"

namespace openarmx_linker_o6_c
{

class O6Rs485Driver : public O6DriverBase
{
public:
  O6Rs485Driver(const std::string & port, int modbus_id, int baudrate);
  ~O6Rs485Driver() override;

  void set_joint_positions(const std::vector<double> & pose) override;
  void set_speed(const std::vector<double> & speed) override;
  void set_torque(const std::vector<double> & torque) override;

  std::vector<int> get_current_status() override;
  std::vector<int> get_speed() override;
  std::vector<int> get_torque() override;
  std::vector<int> get_current() override;
  std::vector<int> get_fault() override;
  std::vector<int> get_temperature() override;
  std::vector<std::vector<float>> get_force() override;
  std::string get_matrix_touch_json() override;
  std::string get_matrix_touch_mass_json() override;
  std::string get_device_info_json() override;

private:
  std::vector<int> read_registers(int address, int count);
  void write_registers(int address, const std::vector<int> & values);
  std::vector<unsigned char> transact(const std::vector<unsigned char> & request, std::size_t min_response_size);
  void configure_serial();

  std::string port_;
  int modbus_id_;
  int baudrate_;
  int fd_{-1};
};

}  // namespace openarmx_linker_o6_c
