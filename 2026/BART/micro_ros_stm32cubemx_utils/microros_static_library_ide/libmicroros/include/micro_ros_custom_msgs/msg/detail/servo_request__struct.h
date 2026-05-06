// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from micro_ros_custom_msgs:msg/ServoRequest.idl
// generated code does not contain a copyright notice

#ifndef MICRO_ROS_CUSTOM_MSGS__MSG__DETAIL__SERVO_REQUEST__STRUCT_H_
#define MICRO_ROS_CUSTOM_MSGS__MSG__DETAIL__SERVO_REQUEST__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/ServoRequest in the package micro_ros_custom_msgs.
typedef struct micro_ros_custom_msgs__msg__ServoRequest
{
  int32_t increment;
  bool zero_in;
  uint32_t status_code;
} micro_ros_custom_msgs__msg__ServoRequest;

// Struct for a sequence of micro_ros_custom_msgs__msg__ServoRequest.
typedef struct micro_ros_custom_msgs__msg__ServoRequest__Sequence
{
  micro_ros_custom_msgs__msg__ServoRequest * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} micro_ros_custom_msgs__msg__ServoRequest__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MICRO_ROS_CUSTOM_MSGS__MSG__DETAIL__SERVO_REQUEST__STRUCT_H_
