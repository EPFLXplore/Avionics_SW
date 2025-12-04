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

extern "C" {
#include "usb_device.h"
#include "usbd_core.h"
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/float32.h>
}

extern USBD_HandleTypeDef hUsbDeviceFS;

// Globals just for clarity here
static rclc_support_t  g_support;
static rcl_node_t      g_node;
static rcl_publisher_t g_pub_mass;
static rcl_publisher_t g_pub_beat;

MicroRosThread::MicroRosThread(QueueHandle_t toRosQueue)
: Thread("MicroRosThread"),
  queue_to_ros(toRosQueue)
{
    setTickDelay(1);
}

void MicroRosThread::init(){
	if (try_connect_and_setup()) {
	    initialized = true;
	}
}

bool MicroRosThread::try_connect_and_setup()
{
    // 1. Setup Custom Transport
    rmw_uros_set_custom_transport(
        true,
        NULL,
        cubemx_transport_open,
        cubemx_transport_close,
        cubemx_transport_write,
        cubemx_transport_read);

    // 2. Setup Allocators
    rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();
    freeRTOS_allocator.allocate      = microros_allocate;
    freeRTOS_allocator.deallocate    = microros_deallocate;
    freeRTOS_allocator.reallocate    = microros_reallocate;
    freeRTOS_allocator.zero_allocate = microros_zero_allocate;
    rcutils_set_default_allocator(&freeRTOS_allocator);

    // 3. Robust Connection Loop
    bool connected = false;
    rcl_allocator_t allocator = rcl_get_default_allocator();

    // Reset support object to zero before starting
    // This is safer than calling fini() on a failed object
    memset(&g_support, 0, sizeof(g_support));

    while (!connected) {

        // Try to initialize
        rcl_ret_t ret = rclc_support_init(&g_support, 0, NULL, &allocator);

        if (ret == RCL_RET_OK) {
            connected = true;
        } else {
            // FIX: Do NOT call rclc_support_fini here.
            // If init failed, the struct is likely garbage.
            // Just ensure it is zeroed before the next attempt.

            // FIX: Do NOT aggressively toggle USB here.
            // Toggling USB while the transport layer (cubemx_transport)
            // might be pending an operation causes HardFaults.

            // Just wait for the Agent to become available.
            vTaskDelay(pdMS_TO_TICKS(1000));

            // Optional: If you MUST reset USB, do it very rarely (e.g., every 10 failures)
            // and ensure transport is closed first.
        }
    }

    // 4. ROS 2 Resource Setup (Only runs if connected = true)
    rclc_node_init_default(&g_node, "cubemx_node", "", &g_support);

    rclc_publisher_init_default(
        &g_pub_mass,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
        "mass");

    rclc_publisher_init_default(
            &g_pub_beat,
            &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
            "beat");

    initialized = true;

    // CRITICAL FIX: Return value strictly required for bool function
    return true;
}

void MicroRosThread::loop()
{
	// A. HEALTH CHECK: Check if connection is active.
	    // The rcl_ok() function checks the health of the core RCL context (g_support).
	    if (initialized && !rcl_context_is_valid(&g_support.context)) {
	        // Disconnected detected! Clean up all resources.
	        initialized = false;
	        rcl_publisher_fini(&g_pub_mass, &g_node);
	        rcl_publisher_fini(&g_pub_beat, &g_node);
	        rcl_node_fini(&g_node);
	        rclc_support_fini(&g_support);

	        // The next block will attempt reconnection.
	    }

	    // B. RECONNECTION/INITIALIZATION ATTEMPT
	    if (!initialized) {
	        // Attempt to connect and set up resources.
	        if (try_connect_and_setup()) {
	            initialized = true;
	        } else {
	            // Wait before retrying the connection to prevent resource hogging.
	            vTaskDelay(pdMS_TO_TICKS(100));
	            return;
	        }
	    }

    SystemMessage msg;

    if (xQueueReceive(queue_to_ros, &msg, pdMS_TO_TICKS(20)) == pdPASS) {
        switch (msg.type) {

        case PacketType::MASS_PACKET: {
            std_msgs__msg__Float32 mass_msg;
            mass_msg.data = msg.data.mass_packet.mass;
            rcl_publish(&g_pub_mass, &mass_msg, NULL);
            break;
        }

        case PacketType::HEARTBEAT: {
        	std_msgs__msg__Float32 beat_msg;
        	beat_msg.data = msg.data.heartbeat.beat;
        	rcl_publish(&g_pub_beat, &beat_msg, NULL);
            break;
        }

        default:
            break;
        }
    }
}

