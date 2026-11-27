cmake_minimum_required(VERSION 3.8)
project(hands_description)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_library(o6_description_core
  src/description/joint_mapping.cpp
  src/description/robot_description.cpp
)
target_include_directories(o6_description_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(o6_description_core PUBLIC cxx_std_17)
ament_target_dependencies(o6_description_core rclcpp)

add_executable(o6_joint_state_mapper_node src/nodes/o6_joint_state_mapper_node.cpp)
target_compile_features(o6_joint_state_mapper_node PUBLIC cxx_std_17)
target_link_libraries(o6_joint_state_mapper_node o6_description_core)
ament_target_dependencies(o6_joint_state_mapper_node rclcpp sensor_msgs)

add_executable(o6_robot_description_node src/nodes/o6_robot_description_node.cpp)
target_compile_features(o6_robot_description_node PUBLIC cxx_std_17)
target_link_libraries(o6_robot_description_node o6_description_core)
ament_target_dependencies(o6_robot_description_node rclcpp)

add_executable(o6_robot_description_cli src/nodes/o6_robot_description_cli.cpp)
target_compile_features(o6_robot_description_cli PUBLIC cxx_std_17)
target_link_libraries(o6_robot_description_cli o6_description_core)

install(TARGETS
  o6_description_core
  EXPORT export_o6_description_core
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS
  o6_joint_state_mapper_node
  o6_robot_description_cli
  o6_robot_description_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(EXPORT export_o6_description_core
  FILE export_o6_description_core.cmake
  NAMESPACE hands_description::
  DESTINATION share/${PROJECT_NAME}/cmake
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY rviz urdf
  DESTINATION share/${PROJECT_NAME}
)

install(FILES package.xml README.md README_CN.md
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_targets(export_o6_description_core HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp)

ament_package()
