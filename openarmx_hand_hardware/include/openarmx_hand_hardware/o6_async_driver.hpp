#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "openarmx_hand_hardware/o6_joint_mapping.hpp"

namespace openarmx_linker_o6_c
{
class O6CanDriver;
}

namespace openarmx_hand_hardware
{

class O6AsyncDriver
{
public:
  O6AsyncDriver(std::string can_interface, int can_id, double update_rate);
  ~O6AsyncDriver();

  bool start();
  void stop();
  void set_command(const O6Pose & command);
  O6Pose state() const;
  bool has_state() const;
  std::string last_error() const;

private:
  void run();

  std::string can_interface_;
  int can_id_;
  double update_rate_;
  std::unique_ptr<openarmx_linker_o6_c::O6CanDriver> driver_;
  std::atomic<bool> running_{false};
  std::thread worker_;

  mutable std::mutex mutex_;
  O6Pose command_{};
  O6Pose state_{};
  bool has_state_{false};
  bool command_changed_{true};
  std::string last_error_;
};

}  // namespace openarmx_hand_hardware
