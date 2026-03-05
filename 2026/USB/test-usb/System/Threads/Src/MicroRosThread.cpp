/*
 * MicroROSThread.cpp
 *
 * Created on: Nov 29, 2025
 * Author: pedro
 */

#include <MicroRosThread.h>
#include "microros_allocators.h"
#include "transport_layer.h"
#include "Thread.h"
#include "System.h"

#include <cstring>   // for memset

// 1. ADD THE CUSTOM MESSAGE HEADER HERE
#include <micro_ros_custom_msgs/msg/beat_packet.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

// Globals just for clarity here
static rclc_support_t    g_support;
static rcl_node_t        g_node;
static rcl_publisher_t   g_pub_mass;
static rcl_publisher_t   g_pub_beat;

// New globals for test subscriber + counter
static rcl_publisher_t      g_pub_subs;
static rcl_subscription_t   g_sub_test;
static std_msgs__msg__Int32 g_sub_msg;
static int32_t              g_counter = 0;


MicroRosThread::MicroRosThread(ThreadsRegistry* registry)
: Thread("MicroRosThread"),
  _reg(registry)
{
    setTickDelay(1);
}

// Callback: each received message increments the counter
void MicroRosThread::TestCallback(const std_msgs__msg__Int32 * msg)
{
    TestPacket testpacket;
    testpacket.ping = msg->data;
    _reg->test->pushCommand(testpacket);
}

void MicroRosThread::init() {
    if (try_connect_and_setup()) {
        initialized = true;
    }
}

void MicroRosThread::updateSubs() {
    if (!initialized || _reg == nullptr) {
            return;
    }

    rcl_ret_t ret = rcl_take(&g_sub_test, &g_sub_msg, NULL, NULL);
    if (ret == RCL_RET_OK) {
        this->TestCallback(&g_sub_msg);
    }
}

void MicroRosThread::updatePubs()
{
    if (!initialized || _reg == nullptr) {
        return;
    }

    // 1) TestTask status -> /subs topic
    if (_reg->test != nullptr) {
        TestPacket st;
        while (_reg->test->popStatus(st)) {
            std_msgs__msg__Int32 msg;
            msg.data = st.ping;
            rcl_publish(&g_pub_subs, &msg, nullptr);
        }
    }

    if (_reg->beat != nullptr) {
         BeatPacket hb; // Your internal Thread registry struct
         while (_reg->beat->popStatus(hb)) {

             // 2. USE THE CUSTOM MESSAGE STRUCT HERE
             micro_ros_custom_msgs__msg__BeatPacket msg;

             // Map your internal data to the custom message fields
             msg.beat = hb.beat;

             // Assuming your internal 'hb' doesn't have status_code yet,
             // we can hardcode it or map it if it exists.
             msg.status_code = 200; // Change this to hb.status_code if you have it!

             rcl_publish(&g_pub_beat, &msg, nullptr);
         }
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

    memset(&g_support, 0, sizeof(g_support));

    while (!connected) {
        rcl_ret_t ret = rclc_support_init(&g_support, 0, NULL, &allocator);

        if (ret == RCL_RET_OK) {
            connected = true;
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 4. ROS 2 Resource Setup
    rclc_node_init_default(&g_node, "cubemx_node", "", &g_support);

    // 3. USE THE CUSTOM TYPE SUPPORT MACRO HERE
    rclc_publisher_init_default(
        &g_pub_beat,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(micro_ros_custom_msgs, msg, BeatPacket),
        "beat");

    rclc_publisher_init_default(
        &g_pub_subs,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "subs");

    rclc_subscription_init_default(
        &g_sub_test,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "test_sub");

    initialized = true;
    g_sub_msg.data = 0;
    return true;
}

void MicroRosThread::loop()
{
    // A. HEALTH CHECK
    if (initialized && !rcl_context_is_valid(&g_support.context)) {
        initialized = false;

        rcl_subscription_fini(&g_sub_test, &g_node);
        rcl_publisher_fini(&g_pub_subs, &g_node);
        rcl_publisher_fini(&g_pub_mass, &g_node);
        rcl_publisher_fini(&g_pub_beat, &g_node);

        rcl_node_fini(&g_node);
        rclc_support_fini(&g_support);
    }

    // B. RECONNECTION
    if (!initialized) {
        if (try_connect_and_setup()) {
            initialized = true;
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
            return;
        }
    }

    if (initialized) {
        updateSubs();
        updatePubs();
    }
}
