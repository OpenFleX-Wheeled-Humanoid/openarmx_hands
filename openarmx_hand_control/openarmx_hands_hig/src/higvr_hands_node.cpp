#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int32_multi_array.hpp"
#include "std_msgs/msg/multi_array_dimension.hpp"
#include "std_msgs/msg/string.hpp"

namespace fs = std::filesystem;

namespace
{
constexpr int kLeftHand = 1;
constexpr int kRightHand = 2;

struct HigvrFrame
{
  int hand_id;
  int finger1;
  int finger2;
  int finger3;
  int finger4;
  int finger5;
  int back_open;

  std::vector<int32_t> values() const
  {
    return {finger1, finger2, finger3, finger4, finger5, back_open};
  }

  std::vector<int32_t> all_values() const
  {
    return {hand_id, finger1, finger2, finger3, finger4, finger5, back_open};
  }
};

std::string trim(const std::string & input)
{
  const auto start = input.find_first_not_of(" \r\n\t");
  if (start == std::string::npos) {
    return "";
  }
  const auto end = input.find_last_not_of(" \r\n\t");
  return input.substr(start, end - start + 1);
}

std::string to_lower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::string shell_quote(const std::string & value)
{
  std::string quoted = "'";
  for (const char c : value) {
    if (c == '\'') {
      quoted += "'\\''";
    } else {
      quoted += c;
    }
  }
  quoted += "'";
  return quoted;
}

bool starts_with(const std::string & value, const std::string & prefix)
{
  return value.rfind(prefix, 0) == 0;
}

speed_t baud_to_termios(const int baud_rate)
{
  switch (baud_rate) {
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 921600:
      return B921600;
    default:
      throw std::invalid_argument("unsupported baud_rate: " + std::to_string(baud_rate));
  }
}

bool parse_int_field(const std::string & field, int * output)
{
  try {
    size_t pos = 0;
    const int value = std::stoi(field, &pos, 10);
    if (pos != field.size()) {
      return false;
    }
    *output = value;
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool parse_higvr_line(const std::string & line, HigvrFrame * frame)
{
  std::vector<std::string> fields;
  std::stringstream ss(trim(line));
  std::string field;
  while (std::getline(ss, field, '\t')) {
    field = trim(field);
    if (!field.empty()) {
      fields.push_back(field);
    }
  }

  if (fields.size() != 7) {
    return false;
  }

  std::vector<int> values;
  values.reserve(fields.size());
  for (const auto & item : fields) {
    int value = 0;
    if (!parse_int_field(item, &value)) {
      return false;
    }
    values.push_back(value);
  }

  if (values[0] != kLeftHand && values[0] != kRightHand) {
    return false;
  }
  for (size_t i = 1; i < values.size(); ++i) {
    if (values[i] < 0 || values[i] > 100) {
      return false;
    }
  }

  *frame = HigvrFrame{
    values[0], values[1], values[2], values[3], values[4], values[5], values[6]};
  return true;
}

std_msgs::msg::Int32MultiArray make_array_msg(
  const std::vector<int32_t> & data, const std::string & label)
{
  std_msgs::msg::Int32MultiArray msg;
  std_msgs::msg::MultiArrayDimension dim;
  dim.label = label;
  dim.size = data.size();
  dim.stride = data.size();
  msg.layout.dim.push_back(dim);
  msg.layout.data_offset = 0;
  msg.data = data;
  return msg;
}
}  // namespace

class HigvrHandsNode : public rclcpp::Node
{
public:
  HigvrHandsNode()
  : Node("higvr_hands_node")
  {
    port_ = declare_parameter<std::string>("port", "auto");
    baud_rate_ = declare_parameter<int>("baud_rate", 115200);
    hand_mode_ = to_lower(declare_parameter<std::string>("hand_mode", "both"));
    reconnect_interval_ = declare_parameter<double>("reconnect_interval", 1.0);
    auto_fix_permissions_ = declare_parameter<bool>("auto_fix_permissions", true);
    publish_raw_ = declare_parameter<bool>("publish_raw", false);
    reverse_left_fingers_ = declare_parameter<bool>("reverse_left_fingers", true);
    reverse_right_fingers_ = declare_parameter<bool>("reverse_right_fingers", false);

    const auto left_topic = declare_parameter<std::string>("left_topic", "/higvr/left_hand");
    const auto right_topic = declare_parameter<std::string>("right_topic", "/higvr/right_hand");
    const auto all_topic = declare_parameter<std::string>("all_topic", "/higvr/hand_frame");
    const auto raw_topic = declare_parameter<std::string>("raw_topic", "/higvr/raw");

    validate_parameters();

    left_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>(left_topic, 10);
    right_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>(right_topic, 10);
    all_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>(all_topic, 10);
    if (publish_raw_) {
      raw_pub_ = create_publisher<std_msgs::msg::String>(raw_topic, 10);
    }

    manager_thread_ = std::thread([this]() { manager_loop(); });

    RCLCPP_INFO(
      get_logger(),
      "HIGVR hand node started: port=%s, baud_rate=%d, hand_mode=%s, auto_fix_permissions=%s",
      port_.c_str(), baud_rate_, hand_mode_.c_str(), auto_fix_permissions_ ? "true" : "false");
  }

  ~HigvrHandsNode() override
  {
    stop_.store(true);
    if (manager_thread_.joinable()) {
      manager_thread_.join();
    }

    std::vector<std::thread> threads;
    {
      std::lock_guard<std::mutex> lock(thread_mutex_);
      threads = std::move(read_threads_);
    }
    for (auto & thread : threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

private:
  void validate_parameters()
  {
    (void)baud_to_termios(baud_rate_);
    if (hand_mode_ != "left" && hand_mode_ != "right" && hand_mode_ != "both") {
      throw std::invalid_argument("hand_mode must be one of: left, right, both");
    }
    if (reconnect_interval_ <= 0.0) {
      throw std::invalid_argument("reconnect_interval must be > 0");
    }
  }

  void manager_loop()
  {
    while (rclcpp::ok() && !stop_.load()) {
      const auto candidates = resolve_port_candidates();
      if (candidates.empty()) {
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 3000, "No HIGVR serial candidates found");
      }

      for (const auto & candidate : candidates) {
        const auto key = device_key(candidate);
        std::lock_guard<std::mutex> lock(thread_mutex_);
        if (active_device_keys_.find(key) != active_device_keys_.end()) {
          continue;
        }
        active_device_keys_.insert(key);
        read_threads_.emplace_back([this, candidate, key]() {
          read_loop_for_port(candidate, key);
        });
      }

      sleep_for_reconnect();
    }
  }

  void read_loop_for_port(const std::string port, const std::string device_key)
  {
    while (rclcpp::ok() && !stop_.load()) {
      int fd = -1;
      try {
        fd = open_and_configure(port);
        RCLCPP_INFO(get_logger(), "Opened HIGVR serial port: %s", port.c_str());
        read_frames(fd, port);
      } catch (const std::exception & exc) {
        RCLCPP_WARN(
          get_logger(), "Serial port %s unavailable: %s. Retrying in %.1fs",
          port.c_str(), exc.what(), reconnect_interval_);
      }

      if (fd >= 0) {
        close(fd);
      }
      sleep_for_reconnect();
    }

    std::lock_guard<std::mutex> lock(thread_mutex_);
    active_device_keys_.erase(device_key);
  }

  std::vector<std::string> resolve_port_candidates() const
  {
    if (port_ != "auto") {
      return {port_};
    }

    std::vector<std::string> preferred;
    std::vector<std::string> fallback;

    if (fs::exists("/dev/higvr_glove")) {
      preferred.push_back("/dev/higvr_glove");
    }

    append_serial_by_id(&preferred, &fallback);
    append_dev_serial_nodes(&fallback);
    unique_keep_order(&preferred);
    unique_keep_order(&fallback);

    preferred.insert(preferred.end(), fallback.begin(), fallback.end());
    unique_by_device(&preferred);
    return preferred;
  }

  void append_serial_by_id(
    std::vector<std::string> * preferred, std::vector<std::string> * fallback) const
  {
    const fs::path by_id_dir("/dev/serial/by-id");
    if (!fs::exists(by_id_dir)) {
      return;
    }

    std::vector<std::string> entries;
    for (const auto & entry : fs::directory_iterator(by_id_dir)) {
      entries.push_back(entry.path().string());
    }
    std::sort(entries.begin(), entries.end());

    for (const auto & path : entries) {
      const auto lower = to_lower(path);
      if (
        lower.find("higvr") != std::string::npos ||
        lower.find("glove") != std::string::npos ||
        lower.find("espressif") != std::string::npos ||
        lower.find("usb_jtag") != std::string::npos ||
        lower.find("serial") != std::string::npos)
      {
        preferred->push_back(path);
      } else {
        fallback->push_back(path);
      }
    }
  }

  void append_dev_serial_nodes(std::vector<std::string> * candidates) const
  {
    const fs::path dev_dir("/dev");
    if (!fs::exists(dev_dir)) {
      return;
    }

    std::vector<std::string> nodes;
    for (const auto & entry : fs::directory_iterator(dev_dir)) {
      const auto name = entry.path().filename().string();
      if (starts_with(name, "ttyACM") || starts_with(name, "ttyUSB")) {
        nodes.push_back(entry.path().string());
      }
    }
    std::sort(nodes.begin(), nodes.end());
    candidates->insert(candidates->end(), nodes.begin(), nodes.end());
  }

  static void unique_keep_order(std::vector<std::string> * values)
  {
    std::vector<std::string> unique;
    for (const auto & value : *values) {
      if (std::find(unique.begin(), unique.end(), value) == unique.end()) {
        unique.push_back(value);
      }
    }
    *values = unique;
  }

  static void unique_by_device(std::vector<std::string> * values)
  {
    std::vector<std::string> unique;
    std::set<std::string> keys;
    for (const auto & value : *values) {
      const auto key = device_key(value);
      if (keys.insert(key).second) {
        unique.push_back(value);
      }
    }
    *values = unique;
  }

  static std::string device_key(const std::string & path)
  {
    try {
      if (fs::exists(path)) {
        return fs::canonical(path).string();
      }
    } catch (const std::exception &) {
    }
    return path;
  }

  int open_and_configure(const std::string & path)
  {
    int fd = ::open(path.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0 && errno == EACCES && auto_fix_permissions_) {
      request_permission_fix(path);
      fd = ::open(path.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    }
    if (fd < 0) {
      throw std::runtime_error(std::string(strerror(errno)));
    }

    try {
      configure_serial(fd);
    } catch (...) {
      close(fd);
      throw;
    }
    return fd;
  }

  void request_permission_fix(const std::string & path)
  {
    RCLCPP_WARN(
      get_logger(),
      "Permission denied for %s. Running sudo chmod a+rw; enter your password in this terminal.",
      path.c_str());
    const std::string command = "sudo chmod a+rw " + shell_quote(path);
    const int ret = std::system(command.c_str());
    if (ret != 0) {
      RCLCPP_WARN(get_logger(), "Permission fix command failed with code %d", ret);
    }
  }

  void configure_serial(const int fd)
  {
    termios options {};
    if (tcgetattr(fd, &options) != 0) {
      throw std::runtime_error(std::string("tcgetattr failed: ") + strerror(errno));
    }

    cfmakeraw(&options);
    const auto baud = baud_to_termios(baud_rate_);
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);
    options.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);
    options.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
    options.c_cflag &= static_cast<tcflag_t>(~CSTOPB);
    options.c_cflag &= static_cast<tcflag_t>(~PARENB);
    options.c_cflag &= static_cast<tcflag_t>(~CSIZE);
    options.c_cflag |= CS8;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
      throw std::runtime_error(std::string("tcsetattr failed: ") + strerror(errno));
    }
    tcflush(fd, TCIFLUSH);
  }

  void read_frames(const int fd, const std::string & port)
  {
    std::string buffer;
    while (rclcpp::ok() && !stop_.load()) {
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(fd, &read_fds);

      timeval timeout {};
      timeout.tv_sec = 0;
      timeout.tv_usec = 200000;

      const int ret = select(fd + 1, &read_fds, nullptr, nullptr, &timeout);
      if (ret < 0) {
        if (errno == EINTR) {
          continue;
        }
        throw std::runtime_error(std::string("select failed: ") + strerror(errno));
      }
      if (ret == 0 || !FD_ISSET(fd, &read_fds)) {
        continue;
      }

      char chunk[1024];
      const ssize_t n = read(fd, chunk, sizeof(chunk));
      if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
          continue;
        }
        throw std::runtime_error(std::string("read failed: ") + strerror(errno));
      }
      if (n == 0) {
        throw std::runtime_error("serial port returned EOF");
      }

      buffer.append(chunk, static_cast<size_t>(n));
      size_t newline_pos = std::string::npos;
      while ((newline_pos = buffer.find('\n')) != std::string::npos) {
        const auto line = buffer.substr(0, newline_pos);
        buffer.erase(0, newline_pos + 1);
        handle_line(trim(line), port);
      }

      if (buffer.size() > 4096) {
        buffer.clear();
      }
    }
  }

  void handle_line(const std::string & line, const std::string & port)
  {
    if (line.empty()) {
      return;
    }

    if (raw_pub_) {
      std_msgs::msg::String raw_msg;
      raw_msg.data = line;
      raw_pub_->publish(raw_msg);
    }

    HigvrFrame frame {};
    if (!parse_higvr_line(line, &frame)) {
      RCLCPP_DEBUG(
        get_logger(), "Ignored invalid HIGVR line from %s: '%s'", port.c_str(), line.c_str());
      return;
    }

    if (!accepts_hand(frame.hand_id)) {
      return;
    }

    const auto values = normalized_values(frame);
    std::vector<int32_t> all_values;
    all_values.reserve(values.size() + 1);
    all_values.push_back(frame.hand_id);
    all_values.insert(all_values.end(), values.begin(), values.end());

    all_pub_->publish(make_array_msg(all_values, "hand_id_plus_values"));
    if (frame.hand_id == kLeftHand) {
      left_pub_->publish(make_array_msg(values, "finger_values_plus_back_open"));
    } else if (frame.hand_id == kRightHand) {
      right_pub_->publish(make_array_msg(values, "finger_values_plus_back_open"));
    }
  }

  std::vector<int32_t> normalized_values(const HigvrFrame & frame) const
  {
    auto values = frame.values();
    const bool should_reverse =
      (frame.hand_id == kLeftHand && reverse_left_fingers_) ||
      (frame.hand_id == kRightHand && reverse_right_fingers_);
    if (should_reverse) {
      std::reverse(values.begin(), values.begin() + 5);
    }
    return values;
  }

  bool accepts_hand(const int hand_id) const
  {
    if (hand_mode_ == "both") {
      return true;
    }
    if (hand_mode_ == "left") {
      return hand_id == kLeftHand;
    }
    return hand_id == kRightHand;
  }

  void sleep_for_reconnect() const
  {
    const auto total_ms = static_cast<int>(reconnect_interval_ * 1000.0);
    const auto step = std::chrono::milliseconds(100);
    auto elapsed = std::chrono::milliseconds(0);
    while (elapsed.count() < total_ms && rclcpp::ok() && !stop_.load()) {
      std::this_thread::sleep_for(step);
      elapsed += step;
    }
  }

  std::string port_;
  int baud_rate_ {};
  std::string hand_mode_;
  double reconnect_interval_ {};
  bool auto_fix_permissions_ {};
  bool publish_raw_ {};
  bool reverse_left_fingers_ {};
  bool reverse_right_fingers_ {};
  std::atomic_bool stop_ {false};
  std::thread manager_thread_;
  std::mutex thread_mutex_;
  std::set<std::string> active_device_keys_;
  std::vector<std::thread> read_threads_;

  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr left_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr right_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr all_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr raw_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<HigvrHandsNode>());
  } catch (const std::exception & exc) {
    RCLCPP_FATAL(rclcpp::get_logger("higvr_hands_node"), "%s", exc.what());
  }
  rclcpp::shutdown();
  return 0;
}
