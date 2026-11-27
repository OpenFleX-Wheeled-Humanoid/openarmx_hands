#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace openarmx_hands_hig
{

struct JoystickFrame
{
  int32_t hand_id{0};
  std::vector<int32_t> values;
  std::vector<int32_t> accessories;
};

struct ButtonEventState
{
  bool initialized{false};
  bool previous{false};
  bool latched{false};
};

struct AxisEventState
{
  bool active{false};
};

bool should_publish_button(bool current, ButtonEventState * state);
bool update_toggle_button(bool current, ButtonEventState * state);
bool should_publish_axes(double x, double y, double threshold, AxisEventState * state);

bool parse_joystick_line(const std::string & line, JoystickFrame * frame);

std::vector<int32_t> joystick_values(const JoystickFrame & frame);
std::vector<int32_t> button_values(const JoystickFrame & frame);

// Return the five accessory fields in the public [X, Y, click, button1, button2] order.
std::vector<int32_t> public_accessory_values(const JoystickFrame & frame, bool is_left);

// Convert a raw 10-bit ADC reading to the VR axis range [-1, 1].
double normalize_axis(
  int32_t raw,
  int32_t center = 510,
  int32_t deadzone = 100,
  int32_t minimum = 0,
  int32_t maximum = 1023,
  bool invert = false);

}  // namespace openarmx_hands_hig
