#include "openarmx_hand_hardware/o6_async_driver.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

#include "openarmx_linker_o6_c/drivers/can_driver.hpp"

namespace openarmx_hand_hardware
{

O6AsyncDriver::O6AsyncDriver(std::string can_interface, int can_id, double update_rate)
: can_interface_(std::move(can_interface)),
  can_id_(can_id),
  update_rate_(std::max(update_rate, 1.0)),
  command_(o6_open_raw_pose()),
  state_(o6_open_raw_pose())
{
}

O6AsyncDriver::~O6AsyncDriver()
{
  stop();
}

bool O6AsyncDriver::start()
{
  if (running_) {
    return true;
  }

  try {
    driver_ = std::make_unique<openarmx_linker_o6_c::O6CanDriver>(
      can_interface_, can_id_);
    const std::vector<double> defaults(kO6JointCount, 255.0);
    driver_->set_speed(defaults);
    driver_->set_torque(defaults);
    const auto open = o6_open_raw_pose();
    driver_->set_joint_positions(std::vector<double>(open.begin(), open.end()));
    for (int attempt = 0; attempt < 10 && !driver_->has_received_state(); ++attempt) {
      driver_->get_current_status();
    }
    if (!driver_->has_received_state()) {
      throw std::runtime_error("O6 did not return a state frame");
    }
  } catch (const std::exception & exception) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_ = exception.what();
    driver_.reset();
    return false;
  }

  running_ = true;
  worker_ = std::thread(&O6AsyncDriver::run, this);
  return true;
}

void O6AsyncDriver::stop()
{
  running_ = false;
  if (worker_.joinable()) {
    worker_.join();
  }
  driver_.reset();
}

void O6AsyncDriver::set_command(const O6Pose & command)
{
  std::lock_guard<std::mutex> lock(mutex_);
  command_ = command;
  command_changed_ = true;
}

O6Pose O6AsyncDriver::state() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}

bool O6AsyncDriver::has_state() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return has_state_;
}

std::string O6AsyncDriver::last_error() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return last_error_;
}

void O6AsyncDriver::run()
{
  const auto period = std::chrono::duration<double>(1.0 / update_rate_);
  auto next_command_refresh = std::chrono::steady_clock::now();

  while (running_) {
    const auto cycle_start = std::chrono::steady_clock::now();
    try {
      O6Pose command{};
      bool send_command = false;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        command = command_;
        send_command = command_changed_ || cycle_start >= next_command_refresh;
        command_changed_ = false;
      }

      if (send_command) {
        driver_->set_joint_positions(std::vector<double>(command.begin(), command.end()));
        next_command_refresh = cycle_start + std::chrono::milliseconds(100);
      }

      const auto raw_state = driver_->get_current_status();
      if (raw_state.size() == kO6JointCount) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (std::size_t index = 0; index < kO6JointCount; ++index) {
          state_[index] = static_cast<double>(raw_state[index]);
        }
        has_state_ = true;
        last_error_.clear();
      }
    } catch (const std::exception & exception) {
      std::lock_guard<std::mutex> lock(mutex_);
      last_error_ = exception.what();
    }

    const auto elapsed = std::chrono::steady_clock::now() - cycle_start;
    if (elapsed < period) {
      std::this_thread::sleep_for(period - elapsed);
    }
  }
}

}  // namespace openarmx_hand_hardware
