cmake_minimum_required(VERSION 3.8)
project(hands_bringup)

find_package(ament_cmake REQUIRED)

install(DIRECTORY bringup launch
  DESTINATION share/${PROJECT_NAME}
)

install(FILES package.xml README.md README_CN.md
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
