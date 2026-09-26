#include "openarmx_linker_o6_c/drivers/rs485_driver.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace openarmx_linker_o6_c
{
namespace
{
constexpr int REG_RD_CURRENT_THUMB_PITCH = 0;
constexpr int REG_RD_CURRENT_THUMB_TORQUE = 6;
constexpr int REG_RD_CURRENT_THUMB_SPEED = 12;
constexpr int REG_RD_THUMB_TEMP = 18;
constexpr int REG_RD_THUMB_ERROR = 24;
constexpr int REG_RD_HAND_FREEDOM = 30;

constexpr int REG_WR_THUMB_PITCH = 0;
constexpr int REG_WR_THUMB_TORQUE = 6;
constexpr int REG_WR_THUMB_SPEED = 12;

unsigned short modbus_crc(const std::vector<unsigned char> & data)
{
  unsigned short crc = 0xFFFF;
  for (const auto byte : data) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x0001) {
        crc = static_cast<unsigned short>((crc >> 1) ^ 0xA001);
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void append_crc(std::vector<unsigned char> & data)
{
  const auto crc = modbus_crc(data);
  data.push_back(static_cast<unsigned char>(crc & 0xFF));
  data.push_back(static_cast<unsigned char>((crc >> 8) & 0xFF));
}

speed_t baud_to_constant(int baudrate)
{
  switch (baudrate) {
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
    default:
      throw std::runtime_error("Unsupported RS485 baudrate: " + std::to_string(baudrate));
  }
}
}  // namespace

O6Rs485Driver::O6Rs485Driver(const std::string & port, int modbus_id, int baudrate)
: port_(port), modbus_id_(modbus_id), baudrate_(baudrate)
{
  fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd_ < 0) {
    throw std::runtime_error("Failed to open RS485 port " + port_ + ": " + std::strerror(errno));
  }
  configure_serial();
}

O6Rs485Driver::~O6Rs485Driver()
{
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

void O6Rs485Driver::configure_serial()
{
  termios tty{};
  if (tcgetattr(fd_, &tty) != 0) {
    throw std::runtime_error("tcgetattr failed: " + std::string(std::strerror(errno)));
  }

  const auto baud = baud_to_constant(baudrate_);
  cfsetospeed(&tty, baud);
  cfsetispeed(&tty, baud);

  tty.c_cflag = static_cast<unsigned int>((tty.c_cflag & ~CSIZE) | CS8);
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 2;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    throw std::runtime_error("tcsetattr failed: " + std::string(std::strerror(errno)));
  }
}

std::vector<unsigned char> O6Rs485Driver::transact(
  const std::vector<unsigned char> & request,
  std::size_t min_response_size)
{
  tcflush(fd_, TCIOFLUSH);
  const auto written = write(fd_, request.data(), request.size());
  if (written != static_cast<ssize_t>(request.size())) {
    throw std::runtime_error("RS485 write failed: " + std::string(std::strerror(errno)));
  }
  tcdrain(fd_);
  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  std::vector<unsigned char> response;
  response.reserve(min_response_size + 16);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
  while (std::chrono::steady_clock::now() < deadline) {
    unsigned char buffer[128]{};
    const auto n = read(fd_, buffer, sizeof(buffer));
    if (n > 0) {
      response.insert(response.end(), buffer, buffer + n);
      if (response.size() >= min_response_size) {
        break;
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }

  if (response.size() < min_response_size) {
    throw std::runtime_error("RS485 response timeout");
  }
  if (response.size() < 4) {
    throw std::runtime_error("RS485 response too short");
  }
  const auto received_crc = static_cast<unsigned short>(response[response.size() - 2]) |
    static_cast<unsigned short>(response[response.size() - 1] << 8);
  const std::vector<unsigned char> without_crc(response.begin(), response.end() - 2);
  if (modbus_crc(without_crc) != received_crc) {
    throw std::runtime_error("RS485 CRC check failed");
  }
  if (response[0] != static_cast<unsigned char>(modbus_id_ & 0xFF)) {
    throw std::runtime_error("RS485 response slave id mismatch");
  }
  if (response[1] & 0x80) {
    throw std::runtime_error("RS485 Modbus exception");
  }
  return response;
}

std::vector<int> O6Rs485Driver::read_registers(int address, int count)
{
  std::vector<unsigned char> req{
    static_cast<unsigned char>(modbus_id_ & 0xFF),
    0x04,
    static_cast<unsigned char>((address >> 8) & 0xFF),
    static_cast<unsigned char>(address & 0xFF),
    static_cast<unsigned char>((count >> 8) & 0xFF),
    static_cast<unsigned char>(count & 0xFF),
  };
  append_crc(req);
  const auto response = transact(req, static_cast<std::size_t>(5 + count * 2));
  if (response[1] != 0x04 || response[2] != count * 2) {
    throw std::runtime_error("RS485 read response format mismatch");
  }
  std::vector<int> values;
  values.reserve(count);
  for (int i = 0; i < count; ++i) {
    const auto high = response[3 + i * 2];
    const auto low = response[4 + i * 2];
    values.push_back((static_cast<int>(high) << 8) | static_cast<int>(low));
  }
  return values;
}

void O6Rs485Driver::write_registers(int address, const std::vector<int> & values)
{
  std::vector<unsigned char> req{
    static_cast<unsigned char>(modbus_id_ & 0xFF),
    0x10,
    static_cast<unsigned char>((address >> 8) & 0xFF),
    static_cast<unsigned char>(address & 0xFF),
    0x00,
    static_cast<unsigned char>(values.size() & 0xFF),
    static_cast<unsigned char>((values.size() * 2) & 0xFF),
  };
  for (const auto value : values) {
    req.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    req.push_back(static_cast<unsigned char>(value & 0xFF));
  }
  append_crc(req);
  const auto response = transact(req, 8);
  if (response[1] != 0x10) {
    throw std::runtime_error("RS485 write response format mismatch");
  }
}

void O6Rs485Driver::set_joint_positions(const std::vector<double> & pose)
{
  write_registers(REG_WR_THUMB_PITCH, six_uint8(pose, 255));
}

void O6Rs485Driver::set_speed(const std::vector<double> & speed)
{
  write_registers(REG_WR_THUMB_SPEED, six_uint8(speed, 255));
}

void O6Rs485Driver::set_torque(const std::vector<double> & torque)
{
  write_registers(REG_WR_THUMB_TORQUE, six_uint8(torque, 255));
}

std::vector<int> O6Rs485Driver::get_current_status() {return read_registers(REG_RD_CURRENT_THUMB_PITCH, 6);}
std::vector<int> O6Rs485Driver::get_speed() {return read_registers(REG_RD_CURRENT_THUMB_SPEED, 6);}
std::vector<int> O6Rs485Driver::get_torque() {return read_registers(REG_RD_CURRENT_THUMB_TORQUE, 6);}
std::vector<int> O6Rs485Driver::get_current() {return std::vector<int>(6, -1);}
std::vector<int> O6Rs485Driver::get_fault() {return read_registers(REG_RD_THUMB_ERROR, 6);}
std::vector<int> O6Rs485Driver::get_temperature() {return read_registers(REG_RD_THUMB_TEMP, 6);}
std::vector<std::vector<float>> O6Rs485Driver::get_force() {return std::vector<std::vector<float>>(4, std::vector<float>(6, -1.0f));}
std::string O6Rs485Driver::get_matrix_touch_json()
{
  return "{\"supported\":false,\"reason\":\"O6 RS485 does not provide matrix touch frames\"}";
}

std::string O6Rs485Driver::get_matrix_touch_mass_json()
{
  return "{\"supported\":false,\"unit\":\"g\",\"thumb_mass\":-1,\"index_mass\":-1,"
         "\"middle_mass\":-1,\"ring_mass\":-1,\"little_mass\":-1}";
}

std::string O6Rs485Driver::get_device_info_json()
{
  const auto values = read_registers(REG_RD_HAND_FREEDOM, 6);
  std::ostringstream ss;
  ss << "{"
     << "\"transport\":\"rs485\","
     << "\"modbus_id\":" << modbus_id_ << ","
     << "\"register_base\":" << REG_RD_HAND_FREEDOM << ","
     << "\"hand_freedom\":" << values[0] << ","
     << "\"hand_version\":" << values[1] << ","
     << "\"hand_number\":" << values[2] << ","
     << "\"hand_direction\":" << values[3] << ","
     << "\"software_version\":" << values[4] << ","
     << "\"hardware_version\":" << values[5]
     << "}";
  return ss.str();
}

}  // namespace openarmx_linker_o6_c
