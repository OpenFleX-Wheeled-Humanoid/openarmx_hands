#include "openarmx_hands_hig/joystick_protocol.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace openarmx_hands_hig
{
namespace
{
std::string trim(const std::string & value)
{
  const auto first = value.find_first_not_of(" \r\n\t");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \r\n\t");
  return value.substr(first, last - first + 1);
}

bool parse_int32(const std::string & text, int32_t * output)
{
  try {
    size_t consumed = 0;
    const long long value = std::stoll(text, &consumed, 10);
    if (consumed != text.size() || value < std::numeric_limits<int32_t>::min() ||
      value > std::numeric_limits<int32_t>::max())
    {
      return false;
    }
    *output = static_cast<int32_t>(value);
    return true;
  } catch (...) {
    return false;
  }
}
}  // namespace

bool should_publish_button(const bool current, ButtonEventState * state)
{
  if (state == nullptr) return false;
  if (!state->initialized) {
    state->initialized = true;
    state->previous = current;
    return current;
  }
  if (current && state->previous) return true;
  if (current == state->previous) return false;
  state->previous = current;
  return true;
}

bool update_toggle_button(const bool current, ButtonEventState * state)
{
  if (state == nullptr) return false;
  const bool rising = current && (!state->initialized || !state->previous);
  state->initialized = true;
  state->previous = current;
  if (!rising) return false;
  state->latched = !state->latched;
  return true;
}

bool should_publish_axes(
  const double x, const double y, const double threshold, AxisEventState * state)
{
  if (state == nullptr || threshold < 0.0) return false;
  const bool active = std::abs(x) > threshold || std::abs(y) > threshold;
  if (!active && !state->active) return false;
  state->active = active;
  return true;
}

bool parse_joystick_line(const std::string & line, JoystickFrame * frame)
{
  if (frame == nullptr) {
    return false;
  }

  std::vector<std::string> fields;
  std::stringstream stream(trim(line));
  std::string field;
  while (std::getline(stream, field, '\t')) {
    field = trim(field);
    if (field.empty()) {
      return false;
    }
    fields.push_back(field);
  }
  if (fields.size() != 12) {
    return false;
  }

  std::vector<int32_t> parsed;
  parsed.reserve(fields.size());
  for (const auto & field_value : fields) {
    int32_t value = 0;
    if (!parse_int32(field_value, &value)) {
      return false;
    }
    parsed.push_back(value);
  }

  // The original six channels retain the documented 0-100 range.
  for (size_t index = 1; index <= 6; ++index) {
    if (parsed[index] < 0 || parsed[index] > 100) {
      return false;
    }
  }

  frame->hand_id = parsed.front();
  frame->values.assign(parsed.begin() + 1, parsed.begin() + 7);
  frame->accessories.assign(parsed.begin() + 7, parsed.end());
  return true;
}

std::vector<int32_t> joystick_values(const JoystickFrame & frame)
{
  if (frame.accessories.size() < 3) return {};
  return {frame.accessories.begin(), frame.accessories.begin() + 3};
}

std::vector<int32_t> button_values(const JoystickFrame & frame)
{
  if (frame.accessories.size() < 5) return {};
  return {frame.accessories.begin() + 3, frame.accessories.begin() + 5};
}

std::vector<int32_t> public_accessory_values(const JoystickFrame & frame, const bool is_left)
{
  if (frame.accessories.size() < 5) return {};
  const auto & a = frame.accessories;
  if (is_left) {
    // Left glove reports axis1=Y, axis2=X and buttons Y,X.
    return {a[1], a[0], a[2], a[3], a[4]};
  }
  // Right glove reports axis1=X, axis2=Y and the observed button order is A,B.
  return {a[0], a[1], a[2], a[4], a[3]};
}

double normalize_axis(
  const int32_t raw,
  const int32_t center,
  const int32_t deadzone,
  const int32_t minimum,
  const int32_t maximum,
  const bool invert)
{
  if (minimum >= maximum || center <= minimum || center >= maximum || deadzone < 0) {
    return 0.0;
  }

  const auto clamped = std::clamp(raw, minimum, maximum);
  const auto lower_deadzone = std::max(minimum, center - deadzone);
  const auto upper_deadzone = std::min(maximum, center + deadzone);
  if (clamped >= lower_deadzone && clamped <= upper_deadzone) {
    return 0.0;
  }

  double normalized = 0.0;
  if (clamped > upper_deadzone) {
    normalized = static_cast<double>(clamped - upper_deadzone) /
      static_cast<double>(maximum - upper_deadzone);
  } else {
    normalized = static_cast<double>(clamped - lower_deadzone) /
      static_cast<double>(lower_deadzone - minimum);
  }
  normalized = std::clamp(normalized, -1.0, 1.0);
  return invert ? -normalized : normalized;
}

}  // namespace openarmx_hands_hig
