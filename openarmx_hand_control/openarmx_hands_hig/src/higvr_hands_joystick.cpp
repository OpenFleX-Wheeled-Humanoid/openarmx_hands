#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "openarmx_hands_hig/joystick_protocol.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int32_multi_array.hpp"
#include "std_msgs/msg/multi_array_dimension.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/string.hpp"

namespace fs = std::filesystem;

namespace
{
using openarmx_hands_hig::JoystickFrame;
using openarmx_hands_hig::parse_joystick_line;
using openarmx_hands_hig::normalize_axis;
using openarmx_hands_hig::public_accessory_values;
using openarmx_hands_hig::should_publish_axes;
using openarmx_hands_hig::should_publish_button;
using openarmx_hands_hig::update_toggle_button;
using openarmx_hands_hig::AxisEventState;
using openarmx_hands_hig::ButtonEventState;

speed_t baud_to_termios(const int baud_rate)
{
  switch (baud_rate) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 921600: return B921600;
    default: throw std::invalid_argument("unsupported baud_rate: " + std::to_string(baud_rate));
  }
}

std::string trim(const std::string & value)
{
  const auto first = value.find_first_not_of(" \r\n\t");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \r\n\t");
  return value.substr(first, last - first + 1);
}

std::string shell_quote(const std::string & value)
{
  std::string quoted = "'";
  for (const char character : value) {
    if (character == '\'') quoted += "'\\''";
    else quoted += character;
  }
  quoted += "'";
  return quoted;
}

std_msgs::msg::Int32MultiArray make_array(const std::vector<int32_t> & values, const std::string & label)
{
  std_msgs::msg::Int32MultiArray message;
  std_msgs::msg::MultiArrayDimension dimension;
  dimension.label = label;
  dimension.size = values.size();
  dimension.stride = values.size();
  message.layout.dim.push_back(dimension);
  message.data = values;
  return message;
}

}  // namespace

class HigvrHandsJoystickNode : public rclcpp::Node
{
public:
  HigvrHandsJoystickNode()
  : Node("higvr_hands_joystick")
  {
    port_ = declare_parameter<std::string>("port", "auto");
    baud_rate_ = declare_parameter<int>("baud_rate", 115200);
    hand_mode_ = declare_parameter<std::string>("hand_mode", "both");
    left_hand_id_ = declare_parameter<int>("left_hand_id", 1);
    right_hand_id_ = declare_parameter<int>("right_hand_id", 2);
    reconnect_interval_ = declare_parameter<double>("reconnect_interval", 1.0);
    auto_fix_permissions_ = declare_parameter<bool>("auto_fix_permissions", true);
    publish_raw_ = declare_parameter<bool>("publish_raw", false);
    publish_vr_topics_ = declare_parameter<bool>("publish_vr_topics", true);
    joystick_center_ = declare_parameter<int>("joystick_center", 510);
    joystick_deadzone_ = declare_parameter<int>("joystick_deadzone", 100);
    joystick_minimum_ = declare_parameter<int>("joystick_minimum", 0);
    joystick_maximum_ = declare_parameter<int>("joystick_maximum", 1023);
    joystick_invert_x_ = declare_parameter<bool>("joystick_invert_x", true);
    joystick_invert_y_ = declare_parameter<bool>("joystick_invert_y", false);
    joystick_invert_left_y_ = declare_parameter<bool>("joystick_invert_left_y", true);
    joystick_invert_right_y_ = declare_parameter<bool>("joystick_invert_right_y", false);
    reverse_left_fingers_ = declare_parameter<bool>("reverse_left_fingers", true);
    // Kept for launch-file compatibility. The new glove uses an explicit
    // right-hand channel map below rather than a simple five-value reversal.
    reverse_right_fingers_ = declare_parameter<bool>("reverse_right_fingers", false);

    (void)baud_to_termios(baud_rate_);
    if (hand_mode_ != "left" && hand_mode_ != "right" && hand_mode_ != "both") {
      throw std::invalid_argument("hand_mode must be one of: left, right, both");
    }
    if (reconnect_interval_ <= 0.0) {
      throw std::invalid_argument("reconnect_interval must be > 0");
    }

    left_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>("/higvr/left_hand", 10);
    right_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>("/higvr/right_hand", 10);
    left_accessories_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>("/higvr/left_accessories", 10);
    right_accessories_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>("/higvr/right_accessories", 10);
    all_pub_ = create_publisher<std_msgs::msg::Int32MultiArray>("/higvr/hand_frame", 10);
    if (publish_raw_) raw_pub_ = create_publisher<std_msgs::msg::String>("/higvr/raw", 10);

    if (publish_vr_topics_) {
      left_joystick_x_pub_ = create_publisher<std_msgs::msg::Float32>("/pico_left_controller/joystick_x", 10);
      left_joystick_y_pub_ = create_publisher<std_msgs::msg::Float32>("/pico_left_controller/joystick_y", 10);
      right_joystick_x_pub_ = create_publisher<std_msgs::msg::Float32>("/pico_right_controller/joystick_x", 10);
      right_joystick_y_pub_ = create_publisher<std_msgs::msg::Float32>("/pico_right_controller/joystick_y", 10);
      left_joystick_click_pub_ = create_publisher<std_msgs::msg::Bool>("/pico_left_controller/joystick_click", 10);
      right_joystick_click_pub_ = create_publisher<std_msgs::msg::Bool>("/pico_right_controller/joystick_click", 10);
      left_button_x_pub_ = create_publisher<std_msgs::msg::Bool>("/pico_left_controller/button_x", 10);
      left_button_y_pub_ = create_publisher<std_msgs::msg::Bool>("/pico_left_controller/button_y", 10);
      right_button_a_pub_ = create_publisher<std_msgs::msg::Bool>("/pico_right_controller/button_a", 10);
      right_button_b_pub_ = create_publisher<std_msgs::msg::Bool>("/pico_right_controller/button_b", 10);
    }

    manager_thread_ = std::thread([this]() { manager_loop(); });
    RCLCPP_INFO(
      get_logger(),
      "HIGVR joystick node started: port=%s baud=%d left_hand_id=%d right_hand_id=%d",
      port_.c_str(), baud_rate_, left_hand_id_, right_hand_id_);
  }

  ~HigvrHandsJoystickNode() override
  {
    stop_.store(true);
    if (manager_thread_.joinable()) manager_thread_.join();
    std::vector<std::thread> threads;
    {
      std::lock_guard<std::mutex> lock(thread_mutex_);
      threads = std::move(read_threads_);
    }
    for (auto & thread : threads) if (thread.joinable()) thread.join();
  }

private:
  std::vector<std::string> resolve_ports() const
  {
    if (port_ != "auto") return {port_};
    std::vector<std::string> result;
    std::set<std::string> unique_paths;
    const auto add_candidate = [&](const fs::path & candidate) {
      std::error_code error;
      const auto canonical = fs::weakly_canonical(candidate, error);
      const std::string resolved = error ? candidate.string() : canonical.string();
      if (unique_paths.insert(resolved).second) result.push_back(resolved);
    };

    if (fs::exists("/dev/higvr_glove")) add_candidate("/dev/higvr_glove");
    const fs::path by_id("/dev/serial/by-id");
    if (fs::exists(by_id)) {
      for (const auto & entry : fs::directory_iterator(by_id)) add_candidate(entry.path());
    }
    for (const auto & directory_entry : fs::directory_iterator("/dev")) {
      const auto name = directory_entry.path().filename().string();
      if (name.rfind("ttyACM", 0) == 0 || name.rfind("ttyUSB", 0) == 0) {
        add_candidate(directory_entry.path());
      }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
  }

  void manager_loop()
  {
    while (rclcpp::ok() && !stop_.load()) {
      for (const auto & candidate : resolve_ports()) {
        std::lock_guard<std::mutex> lock(thread_mutex_);
        if (!active_ports_.insert(candidate).second) continue;
        read_threads_.emplace_back([this, candidate]() { read_loop(candidate); });
      }
      std::this_thread::sleep_for(std::chrono::duration<double>(reconnect_interval_));
    }
  }

  int open_port(const std::string & path)
  {
    int fd = open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0 && errno == EACCES && auto_fix_permissions_) {
      RCLCPP_WARN(
        get_logger(),
        "Permission denied for %s. Running sudo chmod a+rw; enter your password in this terminal.",
        path.c_str());
      const std::string command = "sudo chmod a+rw " + shell_quote(path);
      const int result = std::system(command.c_str());
      if (result != 0) RCLCPP_WARN(get_logger(), "Permission fix command failed with code %d", result);
      fd = open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    }
    if (fd < 0) throw std::runtime_error(std::string("open failed: ") + strerror(errno));
    termios options{};
    if (tcgetattr(fd, &options) != 0) {
      close(fd);
      throw std::runtime_error(std::string("tcgetattr failed: ") + strerror(errno));
    }
    cfmakeraw(&options);
    const auto baud = baud_to_termios(baud_rate_);
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);
    options.c_cflag |= CLOCAL | CREAD;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS;
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
      close(fd);
      throw std::runtime_error(std::string("tcsetattr failed: ") + strerror(errno));
    }
    return fd;
  }

  void read_loop(const std::string & path)
  {
    while (rclcpp::ok() && !stop_.load()) {
      int fd = -1;
      try {
        fd = open_port(path);
        RCLCPP_INFO(get_logger(), "Opened HIGVR joystick serial port: %s", path.c_str());
        read_frames(fd, path);
      } catch (const std::exception & error) {
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 3000, "Serial port %s unavailable: %s",
          path.c_str(), error.what());
      }
      if (fd >= 0) close(fd);
      reset_vr_state_for_path(path);
      if (!stop_.load()) std::this_thread::sleep_for(std::chrono::duration<double>(reconnect_interval_));
    }
    std::lock_guard<std::mutex> lock(thread_mutex_);
    active_ports_.erase(path);
  }

  void read_frames(const int fd, const std::string & path)
  {
    std::string buffer;
    while (rclcpp::ok() && !stop_.load()) {
      fd_set descriptors;
      FD_ZERO(&descriptors);
      FD_SET(fd, &descriptors);
      timeval timeout{0, 200000};
      const int selected = select(fd + 1, &descriptors, nullptr, nullptr, &timeout);
      if (selected < 0) {
        if (errno == EINTR) continue;
        throw std::runtime_error(std::string("select failed: ") + strerror(errno));
      }
      if (selected == 0) continue;
      char chunk[1024];
      const ssize_t count = read(fd, chunk, sizeof(chunk));
      if (count < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
        throw std::runtime_error(std::string("read failed: ") + strerror(errno));
      }
      if (count == 0) throw std::runtime_error("serial port returned EOF");
      buffer.append(chunk, static_cast<size_t>(count));
      size_t newline = std::string::npos;
      while ((newline = buffer.find('\n')) != std::string::npos) {
        const auto line = buffer.substr(0, newline);
        buffer.erase(0, newline + 1);
        handle_line(trim(line), path);
      }
      if (buffer.size() > 4096) buffer.clear();
    }
  }

  void handle_line(const std::string & line, const std::string & path)
  {
    if (line.empty()) return;
    if (raw_pub_) {
      std_msgs::msg::String raw;
      raw.data = line;
      raw_pub_->publish(raw);
    }

    JoystickFrame frame;
    if (!parse_joystick_line(line, &frame)) {
      RCLCPP_DEBUG(get_logger(), "Ignored invalid joystick frame from %s: '%s'", path.c_str(), line.c_str());
      return;
    }

    auto values = frame.values;
    const bool is_left = frame.hand_id == left_hand_id_;
    const bool is_right = frame.hand_id == right_hand_id_;
    if (is_left || is_right) {
      std::lock_guard<std::mutex> lock(vr_state_mutex_);
      hands_by_path_[path].insert(frame.hand_id);
    }
    if (is_right) {
      // Measured new-glove order: raw [index 2, 3, 4, 5, 6, 1]
      // corresponds to semantic [thumb, index, middle, ring, pinky, web].
      const auto glove_values = values;
      values = {
        glove_values[1], glove_values[2], glove_values[3],
        glove_values[4], glove_values[5], glove_values[0]};
    } else if (is_left && reverse_left_fingers_) {
      std::reverse(values.begin(), values.begin() + 5);
    }

    std::vector<int32_t> full;
    full.reserve(1 + frame.values.size() + frame.accessories.size());
    full.push_back(frame.hand_id);
    full.insert(full.end(), values.begin(), values.end());
    full.insert(full.end(), frame.accessories.begin(), frame.accessories.end());
    all_pub_->publish(make_array(full, "hand_id_plus_hand_and_joystick_values"));

    if (is_left && (hand_mode_ == "left" || hand_mode_ == "both")) {
      left_pub_->publish(make_array(values, "finger_values_plus_back_open"));
      // [axis_x, axis_y, joystick_click, Y, X] in the left-hand accessory frame.
      left_accessories_pub_->publish(make_array(
        public_accessory_values(frame, true), "x_y_click_y_button_x_button"));
      publish_vr_left(frame);
    } else if (is_right && (hand_mode_ == "right" || hand_mode_ == "both")) {
      right_pub_->publish(make_array(values, "finger_values_plus_back_open"));
      // [axis_x, axis_y, joystick_click, A, B] in the right-hand accessory frame.
      right_accessories_pub_->publish(make_array(
        public_accessory_values(frame, false), "x_y_click_b_button_a_button"));
      publish_vr_right(frame);
    } else if (!is_left && !is_right) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 3000,
        "Unknown hand id %d; configure left_hand_id/right_hand_id", frame.hand_id);
    }
  }

  std_msgs::msg::Float32 make_axis_message(const int32_t raw, const bool invert) const
  {
    std_msgs::msg::Float32 message;
    message.data = static_cast<float>(normalize_axis(
      raw, joystick_center_, joystick_deadzone_, joystick_minimum_, joystick_maximum_, invert));
    return message;
  }

  std_msgs::msg::Bool make_button_message(const int32_t raw) const
  {
    std_msgs::msg::Bool message;
    message.data = raw != 0;
    return message;
  }

  void publish_vr_left(const JoystickFrame & frame)
  {
    if (!publish_vr_topics_ || frame.accessories.size() < 5) return;
    std::lock_guard<std::mutex> lock(vr_state_mutex_);
    // Left glove hardware reports axis1=Y and axis2=X; buttons are Y then X.
    publish_vr_axes(
      left_state_, left_joystick_x_pub_, left_joystick_y_pub_,
      frame.accessories[1], frame.accessories[0], joystick_invert_left_y_);
    publish_vr_button(
      left_joystick_click_pub_, left_state_.joystick_click_state, frame.accessories[2]);
    publish_vr_button(
      left_button_y_pub_, left_state_.button_y_state, frame.accessories[3]);
    publish_vr_button(
      left_button_x_pub_, left_state_.button_x_state, frame.accessories[4]);
  }

  void publish_vr_right(const JoystickFrame & frame)
  {
    if (!publish_vr_topics_ || frame.accessories.size() < 5) return;
    std::lock_guard<std::mutex> lock(vr_state_mutex_);
    // Right glove hardware reports axis1=X and axis2=Y; observed buttons are A then B.
    publish_vr_axes(
      right_state_, right_joystick_x_pub_, right_joystick_y_pub_,
      frame.accessories[0], frame.accessories[1], joystick_invert_right_y_);
    publish_vr_button(
      right_joystick_click_pub_, right_state_.joystick_click_state, frame.accessories[2]);
    publish_vr_button(
      right_button_a_pub_, right_state_.button_a_state, frame.accessories[3]);
    publish_vr_toggle_button(
      right_button_b_pub_, right_state_.button_b_state, frame.accessories[4]);
  }

  struct VrInputState
  {
    AxisEventState axis_state;
    ButtonEventState joystick_click_state;
    ButtonEventState button_a_state;
    ButtonEventState button_b_state;
    ButtonEventState button_x_state;
    ButtonEventState button_y_state;
  };

  void publish_vr_axes(
    VrInputState & state,
    const rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr & x_pub,
    const rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr & y_pub,
    const int32_t raw_x, const int32_t raw_y, const bool invert_y)
  {
    const auto x = make_axis_message(raw_x, joystick_invert_x_);
    const auto y = make_axis_message(raw_y, invert_y);
    if (!should_publish_axes(x.data, y.data, 0.05, &state.axis_state)) return;
    x_pub->publish(x);
    y_pub->publish(y);
  }

  void publish_vr_button(
    const rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr & publisher,
    ButtonEventState & state, const int32_t raw)
  {
    const bool current = raw != 0;
    if (!should_publish_button(current, &state)) return;
    std_msgs::msg::Bool message;
    message.data = current;
    publisher->publish(message);
  }

  void publish_vr_toggle_button(
    const rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr & publisher,
    ButtonEventState & state, const int32_t raw)
  {
    if (!update_toggle_button(raw != 0, &state)) return;
    std_msgs::msg::Bool message;
    message.data = state.latched;
    publisher->publish(message);
  }

  void reset_vr_state_for_path(const std::string & path)
  {
    if (!publish_vr_topics_) return;
    std::lock_guard<std::mutex> lock(vr_state_mutex_);
    const auto found = hands_by_path_.find(path);
    if (found == hands_by_path_.end()) return;
    if (found->second.count(left_hand_id_) != 0U) {
      reset_vr_hand_state(
        left_state_, left_joystick_x_pub_, left_joystick_y_pub_,
        left_joystick_click_pub_, left_button_x_pub_, left_button_y_pub_,
        nullptr, nullptr);
    }
    if (found->second.count(right_hand_id_) != 0U) {
      reset_vr_hand_state(
        right_state_, right_joystick_x_pub_, right_joystick_y_pub_,
        right_joystick_click_pub_, nullptr, nullptr,
        right_button_a_pub_, right_button_b_pub_);
    }
    hands_by_path_.erase(found);
  }

  void reset_vr_hand_state(
    VrInputState & state,
    const rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr & axis_x_pub,
    const rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr & axis_y_pub,
    const rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr & click_pub,
    const rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr & x_pub,
    const rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr & y_button_pub,
    const rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr & a_pub,
    const rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr & b_pub)
  {
    if (state.axis_state.active) {
      std_msgs::msg::Float32 zero;
      zero.data = 0.0F;
      axis_x_pub->publish(zero);
      axis_y_pub->publish(zero);
    }
    if (state.joystick_click_state.initialized && state.joystick_click_state.previous) {
      std_msgs::msg::Bool released;
      released.data = false;
      click_pub->publish(released);
    }
    if (state.button_a_state.initialized && state.button_a_state.previous && a_pub) {
      std_msgs::msg::Bool released;
      released.data = false;
      a_pub->publish(released);
    }
    if (state.button_b_state.latched && b_pub) {
      std_msgs::msg::Bool released;
      released.data = false;
      b_pub->publish(released);
    }
    if (state.button_x_state.initialized && state.button_x_state.previous && x_pub) {
      std_msgs::msg::Bool released;
      released.data = false;
      x_pub->publish(released);
    }
    if (state.button_y_state.initialized && state.button_y_state.previous && y_button_pub) {
      std_msgs::msg::Bool released;
      released.data = false;
      y_button_pub->publish(released);
    }
    state = VrInputState{};
  }

  std::string port_;
  int baud_rate_{115200};
  std::string hand_mode_;
  int left_hand_id_{1};
  int right_hand_id_{2};
  double reconnect_interval_{1.0};
  bool auto_fix_permissions_{true};
  bool publish_raw_{false};
  bool reverse_left_fingers_{true};
  bool reverse_right_fingers_{false};
  std::atomic_bool stop_{false};
  std::thread manager_thread_;
  std::mutex thread_mutex_;
  std::set<std::string> active_ports_;
  std::vector<std::thread> read_threads_;
  std::mutex vr_state_mutex_;
  std::map<std::string, std::set<int>> hands_by_path_;
  VrInputState left_state_;
  VrInputState right_state_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr left_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr right_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr left_accessories_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr right_accessories_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32MultiArray>::SharedPtr all_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr raw_pub_;
  bool publish_vr_topics_{true};
  int joystick_center_{510};
  int joystick_deadzone_{100};
  int joystick_minimum_{0};
  int joystick_maximum_{1023};
  bool joystick_invert_x_{true};
  bool joystick_invert_y_{false};
  bool joystick_invert_left_y_{true};
  bool joystick_invert_right_y_{false};
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr left_joystick_x_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr left_joystick_y_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr right_joystick_x_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr right_joystick_y_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr left_joystick_click_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr right_joystick_click_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr left_button_x_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr left_button_y_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr right_button_a_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr right_button_b_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<HigvrHandsJoystickNode>());
  } catch (const std::exception & error) {
    RCLCPP_FATAL(rclcpp::get_logger("higvr_hands_joystick"), "%s", error.what());
  }
  rclcpp::shutdown();
  return 0;
}
