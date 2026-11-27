cmake_minimum_required(VERSION 3.8)
project(openarmx_hand_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_executable(joint_state_merger src/joint_state_merger.cpp)
ament_target_dependencies(joint_state_merger rclcpp sensor_msgs)

install(TARGETS joint_state_merger
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY config launch rviz
  DESTINATION share/${PROJECT_NAME}
  PATTERN "__pycache__" EXCLUDE
  PATTERN "*.pyc" EXCLUDE
)

install(FILES README.md README_CN.md LICENSE
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
