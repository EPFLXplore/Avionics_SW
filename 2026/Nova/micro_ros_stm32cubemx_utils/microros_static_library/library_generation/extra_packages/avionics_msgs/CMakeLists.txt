cmake_minimum_required(VERSION 3.5)
project(custom_msg)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/BMS.msg"
  "msg/Heartbeat.msg"
  "msg/LEDRequest.msg"
  "msg/MassPacket.msg"
  "msg/MassRequest.msg"
  "msg/ServoRequest.msg"
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
