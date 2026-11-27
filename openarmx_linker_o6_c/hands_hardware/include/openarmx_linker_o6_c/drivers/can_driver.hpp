#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "openarmx_linker_o6_c/drivers/o6_driver_base.hpp"

namespace openarmx_linker_o6_c
{

using O6TouchMatrix = std::array<std::array<int, 6>, 12>;

class O6CanDriver : public O6DriverBase
{
public:
  O6CanDriver(const std::string & can_channel, int can_id, int bitrate = 1000000);
  ~O6CanDriver() override;

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
  bool has_received_state() const;

private:
  static O6TouchMatrix empty_matrix();
  static std::string vector_json(const std::vector<int> & values);
  static std::string matrix_json(const O6TouchMatrix & matrix);
  static int matrix_mass(const O6TouchMatrix & matrix);

  void open_socket();
  void receive_loop();
  void send_frame(int frame_type, const std::vector<int> & payload, int sleep_ms = 5);
  void process_frame(const std::vector<unsigned char> & data);
  void process_touch_frame(int frame_type, const std::vector<int> & response);
  std::string serial_number_locked() const;

  std::string can_channel_;
  int can_id_;
  int bitrate_;
  int socket_fd_{-1};
  std::atomic<bool> running_{false};
  std::atomic<bool> state_received_{false};
  std::thread receive_thread_;
  std::mutex data_mutex_;

  std::vector<int> x01_{0, 0, 0, 0, 0, 0};
  std::vector<int> x02_{-1, -1, -1, -1, -1, -1};
  std::vector<int> x05_{0, 0, 0, 0, 0, 0};
  std::vector<int> x33_{0, 0, 0, 0, 0, 0};
  std::vector<int> x35_{0, 0, 0, 0, 0, 0};
  std::vector<int> x36_{-1, -1, -1, -1, -1, -1};
  std::vector<int> version_;
  std::vector<int> serial_number_;
  std::vector<int> xb1_{-1, -1};
  std::vector<int> xb2_{-1, -1};
  std::vector<int> xb3_{-1, -1};
  std::vector<int> xb4_{-1, -1};
  std::vector<int> xb5_{-1, -1};
  O6TouchMatrix thumb_matrix_{};
  O6TouchMatrix index_matrix_{};
  O6TouchMatrix middle_matrix_{};
  O6TouchMatrix ring_matrix_{};
  O6TouchMatrix little_matrix_{};
  std::vector<std::vector<float>> force_{
    std::vector<float>(6, -1.0f),
    std::vector<float>(6, -1.0f),
    std::vector<float>(6, -1.0f),
    std::vector<float>(6, -1.0f)};
};

}  // namespace openarmx_linker_o6_c
