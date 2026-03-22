cmake_minimum_required(VERSION 3.8)
project(micro_ros_custom_msgs)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/ServoResquest.msg"
  "msg/MassPacket.msg"
)

ament_package()