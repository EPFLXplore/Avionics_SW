// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from micro_ros_custom_msgs:msg/MassRequest.idl
// generated code does not contain a copyright notice

#ifndef MICRO_ROS_CUSTOM_MSGS__MSG__DETAIL__MASS_REQUEST__FUNCTIONS_H_
#define MICRO_ROS_CUSTOM_MSGS__MSG__DETAIL__MASS_REQUEST__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "micro_ros_custom_msgs/msg/rosidl_generator_c__visibility_control.h"

#include "micro_ros_custom_msgs/msg/detail/mass_request__struct.h"

/// Initialize msg/MassRequest message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * micro_ros_custom_msgs__msg__MassRequest
 * )) before or use
 * micro_ros_custom_msgs__msg__MassRequest__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
bool
micro_ros_custom_msgs__msg__MassRequest__init(micro_ros_custom_msgs__msg__MassRequest * msg);

/// Finalize msg/MassRequest message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
void
micro_ros_custom_msgs__msg__MassRequest__fini(micro_ros_custom_msgs__msg__MassRequest * msg);

/// Create msg/MassRequest message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * micro_ros_custom_msgs__msg__MassRequest__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
micro_ros_custom_msgs__msg__MassRequest *
micro_ros_custom_msgs__msg__MassRequest__create();

/// Destroy msg/MassRequest message.
/**
 * It calls
 * micro_ros_custom_msgs__msg__MassRequest__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
void
micro_ros_custom_msgs__msg__MassRequest__destroy(micro_ros_custom_msgs__msg__MassRequest * msg);

/// Check for msg/MassRequest message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
bool
micro_ros_custom_msgs__msg__MassRequest__are_equal(const micro_ros_custom_msgs__msg__MassRequest * lhs, const micro_ros_custom_msgs__msg__MassRequest * rhs);

/// Copy a msg/MassRequest message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
bool
micro_ros_custom_msgs__msg__MassRequest__copy(
  const micro_ros_custom_msgs__msg__MassRequest * input,
  micro_ros_custom_msgs__msg__MassRequest * output);

/// Initialize array of msg/MassRequest messages.
/**
 * It allocates the memory for the number of elements and calls
 * micro_ros_custom_msgs__msg__MassRequest__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
bool
micro_ros_custom_msgs__msg__MassRequest__Sequence__init(micro_ros_custom_msgs__msg__MassRequest__Sequence * array, size_t size);

/// Finalize array of msg/MassRequest messages.
/**
 * It calls
 * micro_ros_custom_msgs__msg__MassRequest__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
void
micro_ros_custom_msgs__msg__MassRequest__Sequence__fini(micro_ros_custom_msgs__msg__MassRequest__Sequence * array);

/// Create array of msg/MassRequest messages.
/**
 * It allocates the memory for the array and calls
 * micro_ros_custom_msgs__msg__MassRequest__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
micro_ros_custom_msgs__msg__MassRequest__Sequence *
micro_ros_custom_msgs__msg__MassRequest__Sequence__create(size_t size);

/// Destroy array of msg/MassRequest messages.
/**
 * It calls
 * micro_ros_custom_msgs__msg__MassRequest__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
void
micro_ros_custom_msgs__msg__MassRequest__Sequence__destroy(micro_ros_custom_msgs__msg__MassRequest__Sequence * array);

/// Check for msg/MassRequest message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
bool
micro_ros_custom_msgs__msg__MassRequest__Sequence__are_equal(const micro_ros_custom_msgs__msg__MassRequest__Sequence * lhs, const micro_ros_custom_msgs__msg__MassRequest__Sequence * rhs);

/// Copy an array of msg/MassRequest messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_micro_ros_custom_msgs
bool
micro_ros_custom_msgs__msg__MassRequest__Sequence__copy(
  const micro_ros_custom_msgs__msg__MassRequest__Sequence * input,
  micro_ros_custom_msgs__msg__MassRequest__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // MICRO_ROS_CUSTOM_MSGS__MSG__DETAIL__MASS_REQUEST__FUNCTIONS_H_
