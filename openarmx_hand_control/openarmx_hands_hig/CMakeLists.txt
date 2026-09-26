cmake_minimum_required(VERSION 3.8)
project(openarmx_hands_hig)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(higvr_hands_node src/higvr_hands_node.cpp)
target_compile_features(higvr_hands_node PUBLIC cxx_std_17)
ament_target_dependencies(higvr_hands_node rclcpp std_msgs)

add_executable(higvr_hands_joystick
  src/higvr_hands_joystick.cpp
  src/joystick_protocol.cpp
)
target_compile_features(higvr_hands_joystick PUBLIC cxx_std_17)
target_include_directories(higvr_hands_joystick PRIVATE include)
ament_target_dependencies(higvr_hands_joystick rclcpp std_msgs)

install(TARGETS
  higvr_hands_node
  higvr_hands_joystick
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(FILES
  README.md
  package.xml
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_joystick_protocol
    test/test_joystick_protocol.cpp
    src/joystick_protocol.cpp
  )
  target_include_directories(test_joystick_protocol PRIVATE include)
endif()

ament_package()
