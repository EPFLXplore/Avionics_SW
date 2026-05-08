/*
 * MicroROSThread.cpp
 *
 *  Created on: Nov 29, 2025
 *      Author: pedro
 */

// MicroRosThread.cpp
#include <MicroRosThread.h>
#include "microros_allocators.h"
#include "transport_layer.h"
#include "Thread.h"
#include "System.h"

#include <cstring>   // for memset

#include <custom_msg/msg/mass_packet.h>
#include <custom_msg/msg/servo_request.h>
#include <custom_msg/msg/led_request.h>
#include <custom_msg/msg/mass_request.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

static rclc_support_t    g_support;
static rcl_node_t        g_node;
static rcl_publisher_t   g_pub_mass;
static rcl_publisher_t   g_pub_beat;
static rcl_publisher_t      g_pub_subs;
static rcl_subscription_t   g_sub_test;
static rcl_subscription_t   g_sub_servo;
static rcl_subscription_t   g_sub_led;
static rcl_subscription_t   g_sub_mass;
static std_msgs__msg__Int32 g_sub_msg;
static int32_t              g_counter = 0;
static custom_msg__msg__ServoRequest g_servo_req_msg;
static custom_msg__msg__LEDRequest   g_led_req_msg;
static custom_msg__msg__MassRequest g_mass_req_msg;


// 8192 bytes: microROS RCL/RCLC entity creation requires significant stack headroom
MicroRosThread::MicroRosThread(ThreadsRegistry* registry, const char* name, osPriority priority)
: Thread(name, priority, 8192),
  _reg(registry)
{
}


void MicroRosThread::init() {
    // One-time transport and allocator setup — must not be repeated on reconnection
    rmw_uros_set_custom_transport(
        true,
        NULL,
        cubemx_transport_open,
        cubemx_transport_close,
        cubemx_transport_write,
        cubemx_transport_read);

    rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();
    freeRTOS_allocator.allocate      = microros_allocate;
    freeRTOS_allocator.deallocate    = microros_deallocate;
    freeRTOS_allocator.reallocate    = microros_reallocate;
    freeRTOS_allocator.zero_allocate = microros_zero_allocate;
    rcutils_set_default_allocator(&freeRTOS_allocator);

    // Connection is handled in loop() so init() returns immediately
}


void MicroRosThread::destroy_entities() {
    rcl_subscription_fini(&g_sub_test,  &g_node);
    rcl_subscription_fini(&g_sub_servo, &g_node);
    rcl_subscription_fini(&g_sub_led,   &g_node);
    rcl_subscription_fini(&g_sub_mass,  &g_node);
    rcl_publisher_fini(&g_pub_subs, &g_node);
    rcl_publisher_fini(&g_pub_mass, &g_node);
    rcl_publisher_fini(&g_pub_beat, &g_node);
    rcl_node_fini(&g_node);
    rclc_support_fini(&g_support);
}


void MicroRosThread::updateSubs() {
    if (!initialized || _reg == nullptr) return;

//    if (rcl_take(&g_sub_servo, &g_servo_req_msg, NULL, NULL) == RCL_RET_OK) {
//        g_counter++;
//        ServoRequest req;
//        req.id        = g_servo_req_msg.id;
//        req.zero_in   = g_servo_req_msg.zero_in;
//        req.increment = g_servo_req_msg.increment;
//        if (_reg->servo != nullptr) _reg->servo->pushCommand(req);
//    }

    if (rcl_take(&g_sub_led, &g_led_req_msg, NULL, NULL) == RCL_RET_OK) {
        LedRequest req;
        req.id     = 0;
        req.system = g_led_req_msg.system;
        req.mode   = g_led_req_msg.mode;
        if (_reg->leds != nullptr) _reg->leds->pushCommand(req);

        if (_reg->beat != nullptr) {
			BeatPacket hb;
			while (_reg->beat->popStatus(hb)) {
				std_msgs__msg__Float32 msg;
				msg.data = hb.beat;
				rcl_publish(&g_pub_beat, &msg, nullptr);
			}
		}
    }

//    if (rcl_take(&g_sub_mass, &g_mass_req_msg, NULL, NULL) == RCL_RET_OK) {
//        MassRequest req;
//        req.tare = g_mass_req_msg.tare;
//        req.id   = 0;
//        if (_reg->mass != nullptr) _reg->mass->pushCommand(req);
//    }
}

void MicroRosThread::updatePubs() {
    if (!initialized || _reg == nullptr) return;

    if (_reg->mass != nullptr) {
        MassPacket ms;
        while (_reg->mass->popStatus(ms)) {
            custom_msg__msg__MassPacket msg;
            msg.id   = ms.id;
            msg.mass = ms.mass;
            rcl_publish(&g_pub_mass, &msg, nullptr);
        }
    }

//    if (_reg->beat != nullptr) {
//        BeatPacket hb;
//        while (_reg->beat->popStatus(hb)) {
//            std_msgs__msg__Float32 msg;
//            msg.data = hb.beat;
//            rcl_publish(&g_pub_beat, &msg, nullptr);
//        }
//    }
}

bool MicroRosThread::try_connect_and_setup()
{
    rcl_allocator_t allocator = rcl_get_default_allocator();

    // Block until the agent accepts a session.
    // memset inside the loop ensures each retry starts from a clean slate —
    // rclc_support_init() may partially initialize g_support on failure,
    // which would corrupt subsequent calls if not reset.
    while (true) {
        memset(&g_support, 0, sizeof(g_support));
        if (rclc_support_init(&g_support, 0, NULL, &allocator) == RCL_RET_OK) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Discard any stale bytes that accumulated in the ring buffer during
    // failed session attempts, and clear any leftover write-complete flag.
    cubemx_transport_flush();

    if (rclc_node_init_default(&g_node, "cubemx_node", "", &g_support) != RCL_RET_OK) {
        rclc_support_fini(&g_support);
        return false;
    }

    bool ok = true;
    ok &= (rclc_publisher_init_default(&g_pub_beat, &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), "beat") == RCL_RET_OK);
    ok &= (rclc_publisher_init_default(&g_pub_subs, &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "subs") == RCL_RET_OK);
    ok &= (rclc_publisher_init_default(&g_pub_mass, &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(custom_msg, msg, MassPacket), "mass") == RCL_RET_OK);
    ok &= (rclc_subscription_init_default(&g_sub_test, &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "test_sub") == RCL_RET_OK);
    ok &= (rclc_subscription_init_default(&g_sub_servo, &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(custom_msg, msg, ServoRequest), "servo_angle") == RCL_RET_OK);
    ok &= (rclc_subscription_init_default(&g_sub_led, &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(custom_msg, msg, LEDRequest), "led_mode") == RCL_RET_OK);
    ok &= (rclc_subscription_init_default(&g_sub_mass, &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(custom_msg, msg, MassRequest), "mass_tare") == RCL_RET_OK);

    if (!ok) {
        destroy_entities();
        return false;
    }

    g_sub_msg.data = 0;
    return true;
}

void MicroRosThread::loop()
{

    if (initialized && rmw_uros_ping_agent(500, 3) != RMW_RET_OK) {
        destroy_entities();
        initialized = false;
    }

    if (!initialized) {
        if (try_connect_and_setup()) {
            initialized = true;
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
            return;
        }
    }

    updateSubs();
    updatePubs();
}
