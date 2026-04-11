///*
// * MicroROSThread.cpp
// *
// *  Created on: Nov 29, 2025
// *      Author: pedro
// */
//
//// MicroRosThread.cpp
//#include <MicroRosThread.h>
//#include "microros_allocators.h"
//#include "transport_layer.h"
//#include "Thread.h"
//#include "System.h"
//
//#include <cstring>   // for memset
//
//#include <custom_msg/msg/mass_packet.h>
//#include <custom_msg/msg/servo_request.h>
//#include <custom_msg/msg/led_message.h>
//
//extern USBD_HandleTypeDef hUsbDeviceFS;
//
//// Globals just for clarity here
//static rclc_support_t    g_support;
//static rcl_node_t        g_node;
//static rcl_publisher_t   g_pub_mass;
//static rcl_publisher_t   g_pub_beat;
//// New globals for test subscriber + counter
//static rcl_publisher_t      g_pub_subs;   // publishes the counter value
//static rcl_subscription_t   g_sub_test;   // subscriber to bump the counter
//static rcl_subscription_t   g_sub_servo;
//static rcl_subscription_t   g_sub_led;
//static std_msgs__msg__Int32 g_sub_msg;    // storage for incoming sub msg
//static int32_t              g_counter = 0; // incremented on each received msg
//static custom_msg__msg__ServoRequest g_servo_req_msg;
//static custom_msg__msg__LEDMessage   g_led_req_msg;
//
//
//MicroRosThread::MicroRosThread(ThreadsRegistry* registry, const char* name, osPriority priority)
//: Thread(name, priority),
//  _reg(registry)
//{
//}
//
//
//void MicroRosThread::init() {
//    if (try_connect_and_setup()) {
//        initialized = true;
//    }
//}
//
//
//void MicroRosThread::updateSubs() {
//	if (!initialized || _reg == nullptr) {
//	        return;
//	}
//
//    if (rcl_take(&g_sub_servo, &g_servo_req_msg, NULL, NULL) == RCL_RET_OK) {
//    	g_counter++;
//
//        ServoRequest req;
//        req.id        = g_servo_req_msg.id;
//        req.zero_in   = g_servo_req_msg.zero_in;
//        req.increment = g_servo_req_msg.increment;
//
//        if (_reg->servo != nullptr) {
//            _reg->servo->pushCommand(req);
//        }
//    }
//
//    if (rcl_take(&g_sub_led, &g_led_req_msg, NULL, NULL) == RCL_RET_OK) {
//        LedRequest req;
//        req.id     = 0;
//        req.system = g_led_req_msg.system;
//        req.state  = g_led_req_msg.state;
//
//        if (_reg->leds != nullptr) {
//            _reg->leds->pushCommand(req);
//        }
//    }
//}
//
//void MicroRosThread::updatePubs()
//{
//    if (!initialized || _reg == nullptr) {
//        return;
//    }
//
//    if (_reg->mass != nullptr) {
//         MassPacket ms;
//         while (_reg->mass->popStatus(ms)) {
//             custom_msg__msg__MassPacket msg;
//             msg.id   = ms.id;
//             msg.mass = ms.mass;
//             rcl_publish(&g_pub_mass, &msg, nullptr);
//         }
//     }
//
//    if (_reg->beat != nullptr) {
//         BeatPacket hb;
//         while (_reg->beat->popStatus(hb)) {
//             std_msgs__msg__Float32 msg;
//             msg.data = hb.beat;
//             rcl_publish(&g_pub_beat, &msg, nullptr);
//         }
//    }
//
//}
//
//bool MicroRosThread::try_connect_and_setup()
//{
//    // 1. Setup Custom Transport
//    rmw_uros_set_custom_transport(
//        true,
//        NULL,
//        cubemx_transport_open,
//        cubemx_transport_close,
//        cubemx_transport_write,
//        cubemx_transport_read);
//
//    // 2. Setup Allocators
//    rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();
//    freeRTOS_allocator.allocate      = microros_allocate;
//    freeRTOS_allocator.deallocate    = microros_deallocate;
//    freeRTOS_allocator.reallocate    = microros_reallocate;
//    freeRTOS_allocator.zero_allocate = microros_zero_allocate;
//    rcutils_set_default_allocator(&freeRTOS_allocator);
//
//    // 3. Robust Connection Loop
//    bool connected = false;
//    rcl_allocator_t allocator = rcl_get_default_allocator();
//
//    // Reset support object to zero before starting
//    // This is safer than calling fini() on a failed object
//    memset(&g_support, 0, sizeof(g_support));
//
//    while (!connected) {
//
//        // Try to initialize
//        rcl_ret_t ret = rclc_support_init(&g_support, 0, NULL, &allocator);
//
//        if (ret == RCL_RET_OK) {
//            connected = true;
//        } else {
//            // Do NOT call rclc_support_fini here on failed init.
//            // Just wait for the Agent to become available.
//            vTaskDelay(pdMS_TO_TICKS(1000));
//        }
//    }
//
//    // 4. ROS 2 Resource Setup (Only runs if connected = true)
//    rclc_node_init_default(&g_node, "cubemx_node", "", &g_support);
//
//
//    rclc_publisher_init_default(
//        &g_pub_beat,
//        &g_node,
//        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
//        "beat");
//
//    // New publisher: counter on "subs"
//    rclc_publisher_init_default(
//        &g_pub_subs,
//        &g_node,
//        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
//        "subs");
//
//    rclc_publisher_init_default(
//            &g_pub_mass,
//            &g_node,
//            ROSIDL_GET_MSG_TYPE_SUPPORT(custom_msg, msg, MassPacket),
//            "mass");
//
//    // New subscriber: any msg on "test_sub" bumps the counter
//    rclc_subscription_init_default(
//        &g_sub_test,
//        &g_node,
//        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
//        "test_sub");
//
//    rclc_subscription_init_default(
//        &g_sub_servo,
//        &g_node,
//        ROSIDL_GET_MSG_TYPE_SUPPORT(custom_msg, msg, ServoRequest),
//        "servo_angle");
//
//    rclc_subscription_init_default(
//        &g_sub_led,
//        &g_node,
//        ROSIDL_GET_MSG_TYPE_SUPPORT(custom_msg, msg, LEDMessage),
//        "led_mode");
//
//    initialized = true;
//    g_sub_msg.data = 0;
//    // CRITICAL FIX: Return value strictly required for bool function
//    return true;
//}
//
//void MicroRosThread::loop()
//{
//    // A. HEALTH CHECK: Check if connection is active.
//    if (initialized && !rcl_context_is_valid(&g_support.context)) {
//        // Disconnection detected! Clean up all resources.
//        initialized = false;
//
//        // Destroy subscription and publishers
//        rcl_subscription_fini(&g_sub_test, &g_node);
//        rcl_subscription_fini(&g_sub_servo, &g_node);
//        rcl_subscription_fini(&g_sub_led,   &g_node);
//        rcl_publisher_fini(&g_pub_subs, &g_node);
//        rcl_publisher_fini(&g_pub_mass, &g_node);
//        rcl_publisher_fini(&g_pub_beat, &g_node);
//
//        // Finally node + support
//        rcl_node_fini(&g_node);
//        rclc_support_fini(&g_support);
//
//        // The next block will attempt reconnection.
//    }
//
//    // B. RECONNECTION/INITIALIZATION ATTEMPT
//    if (!initialized) {
//        // Attempt to connect and set up resources.
//        if (try_connect_and_setup()) {
//            initialized = true;
//        } else {
//            // Wait before retrying the connection to prevent resource hogging.
//            vTaskDelay(pdMS_TO_TICKS(100));
//            return;
//        }
//    }
//
//    if (initialized) {
//    	updateSubs();
//    	updatePubs();
//    }
//
//
//}
