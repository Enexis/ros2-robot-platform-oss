cmake_minimum_required(VERSION 3.8)
project(unitree_go2_controller)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(unitree_go2_msgs REQUIRED)
find_package(controller_interface REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(urdf REQUIRED)

# Include directories
include_directories(
  include
  include/champ
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Quadruped controller node
add_executable(quadruped_controller_node
  src/quadruped_controller_node.cpp
  src/quadruped_controller.cpp
)

ament_target_dependencies(quadruped_controller_node
  rclcpp
  std_msgs
  geometry_msgs
  sensor_msgs
  nav_msgs
  trajectory_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  unitree_go2_msgs
  controller_interface
  hardware_interface
  urdf
)

# Install targets
install(TARGETS
  quadruped_controller_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install directories
install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY
  include/
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
