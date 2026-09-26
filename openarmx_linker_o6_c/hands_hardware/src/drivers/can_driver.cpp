#include "openarmx_linker_o6_c/drivers/can_driver.hpp"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace openarmx_linker_o6_c
{

O6CanDriver::O6CanDriver(const std::string & can_channel, int can_id, int bitrate)
: can_channel_(can_channel), can_id_(can_id), bitrate_(bitrate)
{
  thumb_matrix_ = empty_matrix();
  index_matrix_ = empty_matrix();
  middle_matrix_ = empty_matrix();
  ring_matrix_ = empty_matrix();
  little_matrix_ = empty_matrix();
  open_socket();
  running_ = true;
  receive_thread_ = std::thread(&O6CanDriver::receive_loop, this);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

O6TouchMatrix O6CanDriver::empty_matrix()
{
  O6TouchMatrix matrix{};
  for (auto & row : matrix) {
    row.fill(-1);
  }
  return matrix;
}

std::string O6CanDriver::vector_json(const std::vector<int> & values)
{
  std::ostringstream ss;
  ss << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      ss << ",";
    }
    ss << values[i];
  }
  ss << "]";
  return ss.str();
}

std::string O6CanDriver::matrix_json(const O6TouchMatrix & matrix)
{
  std::ostringstream ss;
  ss << "[";
  for (std::size_t r = 0; r < matrix.size(); ++r) {
    if (r > 0) {
      ss << ",";
    }
    ss << "[";
    for (std::size_t c = 0; c < matrix[r].size(); ++c) {
      if (c > 0) {
        ss << ",";
      }
      ss << matrix[r][c];
    }
    ss << "]";
  }
  ss << "]";
  return ss.str();
}

int O6CanDriver::matrix_mass(const O6TouchMatrix & matrix)
{
  int total = 0;
  for (const auto & row : matrix) {
    for (const auto value : row) {
      if (value > 0) {
        total += value;
      }
    }
  }
  return total;
}

O6CanDriver::~O6CanDriver()
{
  running_ = false;
  if (socket_fd_ >= 0) {
    shutdown(socket_fd_, SHUT_RDWR);
  }
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
  if (socket_fd_ >= 0) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
}

void O6CanDriver::open_socket()
{
  socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd_ < 0) {
    throw std::runtime_error("Failed to create CAN socket: " + std::string(std::strerror(errno)));
  }

  timeval timeout{};
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    throw std::runtime_error("Failed to set CAN receive timeout: " + std::string(std::strerror(errno)));
  }

  can_filter filter{};
  filter.can_id = static_cast<canid_t>(can_id_);
  filter.can_mask = CAN_SFF_MASK;
  if (setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
    throw std::runtime_error("Failed to set CAN receive filter: " +
      std::string(std::strerror(errno)));
  }

  ifreq ifr{};
  std::snprintf(ifr.ifr_name, IFNAMSIZ, "%s", can_channel_.c_str());
  if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
    throw std::runtime_error("Failed to find CAN interface " + can_channel_ + ": " +
      std::string(std::strerror(errno)));
  }

  sockaddr_can addr{};
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(socket_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    throw std::runtime_error("Failed to bind CAN socket: " + std::string(std::strerror(errno)));
  }
}

void O6CanDriver::send_frame(int frame_type, const std::vector<int> & payload, int sleep_ms)
{
  can_frame frame{};
  frame.can_id = static_cast<canid_t>(can_id_);
  frame.can_dlc = static_cast<__u8>(std::min<std::size_t>(payload.size() + 1, CAN_MAX_DLEN));
  frame.data[0] = static_cast<__u8>(frame_type & 0xFF);
  for (std::size_t i = 0; i + 1 < frame.can_dlc; ++i) {
    frame.data[i + 1] = static_cast<__u8>(payload[i] & 0xFF);
  }

  const auto written = write(socket_fd_, &frame, sizeof(frame));
  if (written != static_cast<ssize_t>(sizeof(frame))) {
    throw std::runtime_error("Failed to send CAN frame: " + std::string(std::strerror(errno)));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
}

void O6CanDriver::set_joint_positions(const std::vector<double> & pose)
{
  send_frame(0x01, six_uint8(pose, 255), 3);
}

void O6CanDriver::set_speed(const std::vector<double> & speed)
{
  const auto data = six_uint8(speed, 255);
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    x05_ = data;
  }
  send_frame(0x05, data, 1);
  send_frame(0x05, data, 1);
}

void O6CanDriver::set_torque(const std::vector<double> & torque)
{
  send_frame(0x02, six_uint8(torque, 255), 5);
}

std::vector<int> O6CanDriver::get_current_status()
{
  send_frame(0x01, {}, 5);
  std::lock_guard<std::mutex> lock(data_mutex_);
  return x01_;
}

bool O6CanDriver::has_received_state() const
{
  return state_received_.load();
}

std::vector<int> O6CanDriver::get_speed()
{
  send_frame(0x05, {}, 2);
  std::lock_guard<std::mutex> lock(data_mutex_);
  return x05_;
}

std::vector<int> O6CanDriver::get_torque()
{
  send_frame(0x02, {}, 10);
  std::lock_guard<std::mutex> lock(data_mutex_);
  return x02_;
}

std::vector<int> O6CanDriver::get_current()
{
  send_frame(0x36, {}, 5);
  std::lock_guard<std::mutex> lock(data_mutex_);
  return x36_;
}

std::vector<int> O6CanDriver::get_fault()
{
  send_frame(0x35, {}, 5);
  std::lock_guard<std::mutex> lock(data_mutex_);
  return x35_;
}

std::vector<int> O6CanDriver::get_temperature()
{
  send_frame(0x33, {}, 5);
  std::lock_guard<std::mutex> lock(data_mutex_);
  return x33_;
}

std::vector<std::vector<float>> O6CanDriver::get_force()
{
  send_frame(0x20, {}, 10);
  send_frame(0x21, {}, 10);
  send_frame(0x22, {}, 10);
  send_frame(0x23, {}, 10);
  std::lock_guard<std::mutex> lock(data_mutex_);
  return force_;
}

std::string O6CanDriver::get_matrix_touch_json()
{
  constexpr int touch_code = 0xA4;
  send_frame(0xB1, {touch_code}, 10);
  send_frame(0xB2, {touch_code}, 10);
  send_frame(0xB3, {touch_code}, 10);
  send_frame(0xB4, {touch_code}, 10);
  send_frame(0xB5, {touch_code}, 10);

  std::lock_guard<std::mutex> lock(data_mutex_);
  std::ostringstream ss;
  ss << "{"
     << "\"supported\":true,"
     << "\"thumb_matrix\":" << matrix_json(thumb_matrix_) << ","
     << "\"index_matrix\":" << matrix_json(index_matrix_) << ","
     << "\"middle_matrix\":" << matrix_json(middle_matrix_) << ","
     << "\"ring_matrix\":" << matrix_json(ring_matrix_) << ","
     << "\"little_matrix\":" << matrix_json(little_matrix_)
     << "}";
  return ss.str();
}

std::string O6CanDriver::get_matrix_touch_mass_json()
{
  std::lock_guard<std::mutex> lock(data_mutex_);
  std::ostringstream ss;
  ss << "{"
     << "\"unit\":\"g\","
     << "\"thumb_mass\":" << matrix_mass(thumb_matrix_) << ","
     << "\"index_mass\":" << matrix_mass(index_matrix_) << ","
     << "\"middle_mass\":" << matrix_mass(middle_matrix_) << ","
     << "\"ring_mass\":" << matrix_mass(ring_matrix_) << ","
     << "\"little_mass\":" << matrix_mass(little_matrix_)
     << "}";
  return ss.str();
}

std::string O6CanDriver::serial_number_locked() const
{
  std::string out;
  for (const auto value : serial_number_) {
    if (value >= 32 && value <= 126) {
      out.push_back(static_cast<char>(value));
    }
  }
  return out;
}

std::string O6CanDriver::get_device_info_json()
{
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    serial_number_.clear();
  }
  send_frame(0x64, {}, 100);
  bool needs_fallback = false;
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    needs_fallback = version_.empty();
  }
  if (needs_fallback) {
    send_frame(0xC2, {}, 100);
  }
  send_frame(0xC0, {}, 100);

  std::lock_guard<std::mutex> lock(data_mutex_);
  std::ostringstream ss;
  ss << "{"
     << "\"transport\":\"can\","
     << "\"can\":\"" << can_channel_ << "\","
     << "\"can_id\":" << can_id_ << ","
     << "\"version\":" << vector_json(version_) << ","
     << "\"serial_number\":\"" << serial_number_locked() << "\","
     << "\"matrix_touch_code\":164"
     << "}";
  return ss.str();
}

void O6CanDriver::receive_loop()
{
  while (running_) {
    can_frame frame{};
    const auto nbytes = read(socket_fd_, &frame, sizeof(frame));
    if (nbytes < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      if (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      continue;
    }
    if (frame.can_id != static_cast<canid_t>(can_id_) || frame.can_dlc == 0) {
      continue;
    }
    std::vector<unsigned char> data(frame.data, frame.data + frame.can_dlc);
    process_frame(data);
  }
}

void O6CanDriver::process_frame(const std::vector<unsigned char> & data)
{
  if (data.empty()) {
    return;
  }
  const auto frame_type = data[0];
  std::vector<int> response;
  response.reserve(data.size() - 1);
  for (std::size_t i = 1; i < data.size(); ++i) {
    response.push_back(static_cast<int>(data[i]));
  }
  if (response.empty()) {
    return;
  }

  std::lock_guard<std::mutex> lock(data_mutex_);
  switch (frame_type) {
    case 0x01:
      x01_ = response;
      state_received_ = true;
      break;
    case 0x02:
      x02_ = response;
      break;
    case 0x05:
      x05_ = response;
      break;
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23: {
        const auto index = static_cast<std::size_t>(frame_type - 0x20);
        force_[index].clear();
        for (const auto value : response) {
          force_[index].push_back(static_cast<float>(value));
        }
      }
      break;
    case 0x33:
      x33_ = response;
      break;
    case 0x35:
      x35_ = response;
      break;
    case 0x36:
      x36_ = response;
      break;
    case 0x64:
    case 0xC2:
      version_ = response;
      break;
    case 0xC0:
      if (!response.empty() && response[0] >= 0 && response[0] <= 3) {
        serial_number_.insert(serial_number_.end(), response.begin() + 1, response.end());
      }
      break;
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
      process_touch_frame(frame_type, response);
      break;
    default:
      break;
  }
}

void O6CanDriver::process_touch_frame(int frame_type, const std::vector<int> & response)
{
  if (response.empty()) {
    return;
  }

  std::vector<int> * touch_summary = nullptr;
  O6TouchMatrix * matrix = nullptr;
  switch (frame_type) {
    case 0xB1:
      touch_summary = &xb1_;
      matrix = &thumb_matrix_;
      break;
    case 0xB2:
      touch_summary = &xb2_;
      matrix = &index_matrix_;
      break;
    case 0xB3:
      touch_summary = &xb3_;
      matrix = &middle_matrix_;
      break;
    case 0xB4:
      touch_summary = &xb4_;
      matrix = &ring_matrix_;
      break;
    case 0xB5:
      touch_summary = &xb5_;
      matrix = &little_matrix_;
      break;
    default:
      return;
  }

  if (response.size() == 2) {
    *touch_summary = response;
    return;
  }
  if (response.size() != 5 && response.size() != 7) {
    return;
  }

  static const std::map<int, std::size_t> matrix_map{
    {0, 0}, {16, 1}, {32, 2}, {48, 3}, {64, 4}, {80, 5},
    {96, 6}, {112, 7}, {128, 8}, {144, 9}, {160, 10}, {176, 11},
  };
  const auto found = matrix_map.find(response[0]);
  if (found == matrix_map.end()) {
    return;
  }
  auto & row = (*matrix)[found->second];
  const auto count = std::min<std::size_t>(row.size(), response.size() - 1);
  for (std::size_t i = 0; i < count; ++i) {
    row[i] = response[i + 1];
  }
}

}  // namespace openarmx_linker_o6_c
