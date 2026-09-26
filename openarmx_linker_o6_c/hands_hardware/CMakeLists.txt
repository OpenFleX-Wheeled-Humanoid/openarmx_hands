cmake_minimum_required(VERSION 3.8)
project(hands_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(hands_description REQUIRED)

add_library(o6_hardware_core
  src/drivers/can_driver.cpp
  src/drivers/o6_driver_base.cpp
  src/drivers/rs485_driver.cpp
)
set_target_properties(o6_hardware_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(o6_hardware_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(o6_hardware_core PUBLIC cxx_std_17)
ament_target_dependencies(o6_hardware_core rclcpp)

add_executable(o6_driver_node src/nodes/o6_driver_node.cpp)
target_compile_features(o6_driver_node PUBLIC cxx_std_17)
target_link_libraries(o6_driver_node o6_hardware_core hands_description::o6_description_core)
ament_target_dependencies(o6_driver_node rclcpp sensor_msgs std_msgs hands_description)

install(TARGETS
  o6_hardware_core
  EXPORT export_o6_hardware_core
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS
  o6_driver_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(EXPORT export_o6_hardware_core
  FILE export_o6_hardware_core.cmake
  NAMESPACE hands_hardware::
  DESTINATION share/${PROJECT_NAME}/cmake
)

install(DIRECTORY include/
  DESTINATION include
)

install(FILES package.xml README.md README_CN.md
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_targets(export_o6_hardware_core HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp hands_description)

ament_package()
