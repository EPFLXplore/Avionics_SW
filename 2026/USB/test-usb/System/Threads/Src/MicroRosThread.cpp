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

#include <micro_ros_custom_msgs/msg/mass_packet.h>
#include <micro_ros_custom_msgs/msg/servo_request.h>
#include <micro_ros_custom_msgs/msg/led_request.h>
#include <micro_ros_custom_msgs/msg/mass_request.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

// Globals just for clarity here
static rclc_support_t    g_support;
static rcl_node_t        g_node;

static rcl_publisher_t   g_pub_mass;
static rcl_publisher_t   g_pub_beat;
static rcl_publisher_t   g_pub_servo_request;
static rcl_publisher_t   g_pub_mass_request;
static rcl_publisher_t   g_pub_led_request;

// New globals for test subscriber + counter
static rcl_publisher_t      g_pub_subs;   // publishes the counter value
static rcl_subscription_t   g_sub_test;   // subscriber to bump the counter
static rcl_subscription_t   g_sub_servo;
static rcl_subscription_t   g_sub_mass;
static rcl_subscription_t   g_sub_led;

static std_msgs__msg__Int32 g_sub_msg;    // storage for incoming sub msg
static int32_t              g_counter = 0; // incremented on each received msg

static micro_ros_custom_msgs__msg__ServoRequest g_servo_req_msg;
static micro_ros_custom_msgs__msg__LEDRequest g_led_req_msg;
static micro_ros_custom_msgs__msg__MassRequest g_mass_req_msg;


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

    if (rcl_take(&g_sub_servo, &g_servo_req_msg, NULL, NULL) == RCL_RET_OK) {

        ServoRequest req;
        req.zero_in = g_servo_req_msg.zero_in;
        req.increment = g_servo_req_msg.increment;
        req.status_code = g_servo_req_msg.status_code; // Map this too!

        if (_reg->servo != nullptr) {
            _reg->servo->pushCommand(req);
        }
    }

    // --- MASS SUB (Tare) ---
        if (rcl_take(&g_sub_mass, &g_mass_req_msg, NULL, NULL) == RCL_RET_OK) {
            MassRequest req;
            req.tare = g_mass_req_msg.tare;
            // Note: On envoie la commande au thread Mass
            if (_reg->mass != nullptr) _reg->mass->pushCommand(req);
        }

        // --- LED SUB ---
      /*      if (rcl_take(&g_sub_led, &g_led_req_msg, NULL, NULL) == RCL_RET_OK) {
                LedRequest req;
                req.system = g_led_req_msg.system;
                req.state = g_led_req_msg.state;
                if (_reg->led != nullptr) _reg->led->pushCommand(req);
            }*/
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
            msg.data = st.ping;   // this holds the accumulated counter
            rcl_publish(&g_pub_subs, &msg, nullptr);
        }
    }

    if (_reg->mass != nullptr) {
         MassPacket ms;
         while (_reg->mass->popStatus(ms)) {
             micro_ros_custom_msgs__msg__MassPacket msg;
             msg.mass = ms.mass;
             msg.status_code = 15;
             rcl_publish(&g_pub_mass, &msg, nullptr);
         }
     }

    if (_reg->beat != nullptr) {
         BeatPacket hb;
         while (_reg->beat->popStatus(hb)) {
             std_msgs__msg__Float32 msg;
             msg.data = hb.beat;
             rcl_publish(&g_pub_beat, &msg, nullptr);
         }
    }

    // --- MASS TARE CONFIRMATION ---
    /*    if (_reg->mass != nullptr) {
            MassRequest ms_req;
            // On vérifie s'il y a des réponses de type MassRequest dans la queue status
            while (_reg->mass->popStatus(ms_req)) {
                micro_ros_custom_msgs__msg__MassRequest msg;
                msg.tare = ms_req.tare;
                msg.status_code = 200; // Exemple: OK
                rcl_publish(&g_pub_mass_request, &msg, nullptr);
            }
        }*/

        // --- LED STATUS/CONFIRMATION ---
     /*       if (_reg->led != nullptr) {
                LedRequest lr;
                while (_reg->led->popStatus(lr)) {
                    micro_ros_custom_msgs__msg__LEDRequest msg;
                    msg.system = lr.system;
                    msg.state = lr.state;
                    rcl_publish(&g_pub_led_request, &msg, nullptr);
                }
            }*/


   /*if (_reg->servo != nullptr) {
         BeatPacket hb;
         while (_reg->beat->popStatus(hb)) {
             std_msgs__msg__Float32 msg;
             msg.data = hb.beat;
             rcl_publish(&g_pub_beat, &msg, nullptr);
         }
    }*/
}

/*void MicroRosThread::ServoCallback(const std_msgs__msg__Int32 * msg)
{
    ServoRequest req;
    req.increment = msg->data; // Using the Int32 data as the target angle
    req.zero_in = false;

    // Push the command to the ServoThread queue
    if (_reg->servo != nullptr) {
        _reg->servo->pushCommand(req);
    }
}
*/

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
            // Do NOT call rclc_support_fini here on failed init.
            // Just wait for the Agent to become available.
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 4. ROS 2 Resource Setup (Only runs if connected = true)
    rclc_node_init_default(&g_node, "cubemx_node", "", &g_support);


    rclc_publisher_init_default(
        &g_pub_beat,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
        "beat");

    // New publisher: counter on "subs"
    rclc_publisher_init_default(
        &g_pub_subs,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "subs");

    rclc_publisher_init_default(
            &g_pub_mass,
            &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(micro_ros_custom_msgs, msg, MassPacket),
            "Mass_Info");

    rclc_publisher_init_default(
                &g_pub_mass_request,
                &g_node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(micro_ros_custom_msgs, msg, MassRequest),
                "Mass_Tare_Status");

    /*rclc_publisher_init_default(
                &g_pub_led_request,
                &g_node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(micro_ros_custom_msgs, msg, LEDRequest),
                "LED_Status");*/

    // New subscriber: any msg on "test_sub" bumps the counter
    rclc_subscription_init_default(
        &g_sub_test,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "test_sub");

    rclc_subscription_init_default(
        &g_sub_servo,
        &g_node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(micro_ros_custom_msgs, msg, ServoRequest),
        "Servo_angle_request"); // Topic name

    rclc_subscription_init_default(
            &g_sub_mass,
            &g_node,
            ROSIDL_GET_MSG_TYPE_SUPPORT(micro_ros_custom_msgs, msg, MassRequest),
            "Mass_Tare_Command");

    /*rclc_subscription_init_default(
                &g_sub_led,
                &g_node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(micro_ros_custom_msgs, msg, LEDRequest),
                "LED_Mode_Request");*/

    initialized = true;
    g_sub_msg.data = 0;
    // CRITICAL FIX: Return value strictly required for bool function
    return true;
}

void MicroRosThread::loop()
{
    // A. HEALTH CHECK: Check if connection is active.
    if (initialized && !rcl_context_is_valid(&g_support.context)) {
        // Disconnection detected! Clean up all resources.
        initialized = false;

        // Destroy subscription and publishers
        rcl_subscription_fini(&g_sub_test, &g_node);
        rcl_subscription_fini(&g_sub_servo, &g_node);
        rcl_subscription_fini(&g_sub_mass, &g_node);
        //rcl_subscription_fini(&g_sub_led, &g_node);


        rcl_publisher_fini(&g_pub_subs, &g_node);
        rcl_publisher_fini(&g_pub_mass, &g_node);
        rcl_publisher_fini(&g_pub_beat, &g_node);
        //rcl_publisher_fini(&g_pub_led_request, &g_node);
        //rcl_publisher_fini(&g_pub_mass_request, &g_node);

        // Finally node + support
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

    if (initialized) {
    	updateSubs();
    	updatePubs();
    }


}
