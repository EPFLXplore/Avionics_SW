cmake_minimum_required(VERSION 3.8)
project(custom_msg)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/avionics/ServoRequest.msg"
  "msg/avionics/MassPacket.msg"
  "msg/avionics/BMS.msg"
  "msg/avionics/LEDRequest.msg"
  "msg/avionics/Heartbeat.msg"
  "msg/avionics/MassRequest.msg"
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
