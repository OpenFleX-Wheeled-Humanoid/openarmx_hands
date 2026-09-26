cmake_minimum_required(VERSION 3.8)
project(openarmx_hand_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(openarmx_hardware REQUIRED)
find_package(openarmx_can REQUIRED)
find_package(hands_hardware REQUIRED)

add_library(${PROJECT_NAME} SHARED
  src/o6_async_driver.cpp
  src/o6_joint_mapping.cpp
  src/openarmx_o6_system.cpp
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME}
  hands_hardware::o6_hardware_core
  openarmx_can::openarmx_can
)
ament_target_dependencies(${PROJECT_NAME}
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  openarmx_hardware
  openarmx_can
  hands_hardware
)

add_executable(o6_command_adapter
  src/o6_command_adapter.cpp
  src/o6_joint_mapping.cpp
)
target_compile_features(o6_command_adapter PUBLIC cxx_std_17)
target_include_directories(o6_command_adapter PRIVATE include)
ament_target_dependencies(o6_command_adapter rclcpp sensor_msgs std_msgs)

pluginlib_export_plugin_description_file(
  hardware_interface openarmx_hand_hardware.xml)

install(TARGETS
  ${PROJECT_NAME}
  o6_command_adapter
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(FILES
  openarmx_hand_hardware.xml
  README.md
  README_CN.md
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  openarmx_hardware
  openarmx_can
  hands_hardware
)

ament_package()
