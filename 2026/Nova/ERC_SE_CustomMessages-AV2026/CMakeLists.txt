cmake_minimum_required(VERSION 3.5)
project(custom_msg)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)

find_package(rosidl_default_generators REQUIRED)

find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)

find_package(sensor_msgs REQUIRED)

# install config directory
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})


set(message_files
  "msg/not_hd/NodeStateArray.msg"

  # HD
  "msg/hd/JointSpaceCmd.msg"
  "msg/hd/MotorCommand.msg"
  "msg/hd/Object.msg"
  "msg/hd/PoseGoal.msg"
  "msg/hd/ArucoObject.msg"
  "msg/hd/CompressedRGBD.msg"
  "msg/hd/MotorCommands.msg"
  "msg/hd/OldMotorStatus.msg"
  "msg/hd/MotorStatushd.msg"
  "msg/hd/HDGoal.msg"
  "msg/hd/Rock.msg"
  "msg/hd/NamedPose.msg"
)

set(message_avionics
  "msg/avionics/BMS.msg"
  "msg/avionics/LEDRequest.msg"
  "msg/avionics/MassPacket.msg"
  "msg/avionics/ServoRequest.msg"
  "msg/avionics/Heartbeat.msg"
  "msg/avionics/MassRequest.msg"
)

set(message_nav
  "msg/not_hd/Motorcmds.msg"
  "msg/not_hd/NavFeedback.msg"
  "msg/not_hd/Wheelstatus.msg"
  "msg/not_hd/Statussteering.msg"
  "msg/not_hd/MotorStatus.msg"
  "msg/not_hd/ArucoMarkers.msg"
)


rosidl_generate_interfaces(${PROJECT_NAME}
  "action/HDManipulation.action"
  "action/DrillCmd.action"
  "action/DrillMain.action"
  "action/NAVReachGoal.action"
  "action/NewHDGoal.action"
  "srv/ChangeModeCamera.srv"
  "srv/ChangeModeSystem.srv"
  "srv/DroneState.srv"
  "srv/DrillMode.srv"
  "srv/ControlStationSelection.srv"
  "srv/RockPipelineRequest.srv"
  "srv/ArucoPipelineRequest.srv"
  "srv/SandPipelineRequest.srv"
  "srv/QrPipelineRequest.srv"
  "srv/SegmentPipelineRequest.srv"
  "srv/HumanVerification.srv"
  "srv/HDMode.srv"
  "srv/CameraParams.srv"
  "msg/not_hd/ScFSMStatusDrill.msg"
  "msg/not_hd/ScMotorCmds.msg"
  "msg/not_hd/ScMotorStatus.msg"
  "msg/not_hd/ScDrillCmds.msg"
  ${message_files}
  ${message_nav}
  ${message_avionics}
  DEPENDENCIES builtin_interfaces sensor_msgs std_msgs geometry_msgs nav_msgs
 )
ament_export_dependencies(rosidl_default_runtime)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()