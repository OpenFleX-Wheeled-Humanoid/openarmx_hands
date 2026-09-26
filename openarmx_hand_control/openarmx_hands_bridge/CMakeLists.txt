cmake_minimum_required(VERSION 3.8)
project(openarmx_hands_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(higvr_to_o6_node src/higvr_to_o6_node.cpp)
target_compile_features(higvr_to_o6_node PUBLIC cxx_std_17)
ament_target_dependencies(higvr_to_o6_node rclcpp sensor_msgs std_msgs)

install(TARGETS
  higvr_to_o6_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

install(FILES
  README.md
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
