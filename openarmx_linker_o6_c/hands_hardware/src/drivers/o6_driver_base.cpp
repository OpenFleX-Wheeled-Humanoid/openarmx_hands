#include "openarmx_linker_o6_c/drivers/o6_driver_base.hpp"

#include <algorithm>
#include <cmath>

namespace openarmx_linker_o6_c
{

std::vector<int> six_uint8(const std::vector<double> & values, int default_value)
{
  std::vector<int> data(6, std::clamp(default_value, 0, 255));
  const auto count = std::min<std::size_t>(values.size(), 6);
  for (std::size_t i = 0; i < count; ++i) {
    data[i] = std::clamp(static_cast<int>(std::lround(values[i])), 0, 255);
  }
  return data;
}

}  // namespace openarmx_linker_o6_c
