/*
 * MicroROSThread.cpp
 *
 * Modified on: Mar 16, 2026
 * Author: pedro / Gemini
 */

#include <MicroRosThread.h>
#include "microros_allocators.h"
#include "transport_layer.h"
#include "Thread.h"
#include "System.h"
#include <cstring>   // for memset
#include <std_msgs/msg/string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

// Globals
static rclc_support_t    g_support;
static rcl_node_t        g_node;
static rcl_publisher_t   g_pub_mass;
static rcl_publisher_t   g_pub_beat;
static rcl_publisher_t   g_pub_adc;    // <--- NEW: ADC Publisher
static rcl_publisher_t g_pub_hat_name;
static std_msgs__msg__String g_hat_msg;

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

	// 2) Beat status -> /beat topic
	if (_reg->beat != nullptr) {
		 BeatPacket hb;
		 while (_reg->beat->popStatus(hb)) {
			 std_msgs__msg__Float32 msg;
			 msg.data = hb.beat;
			 rcl_publish(&g_pub_beat, &msg, nullptr);
		 }
	}

	// 3) NEW: Analog status -> /adc topic
	if (_reg->analog != nullptr) {
		AnalogPacket ap;
		while (_reg->analog->popStatus(ap)) {
			// 1. Publish the raw voltage to /adc as usual
			std_msgs__msg__Float32 v_msg;
			v_msg.data = ap.voltage;
			rcl_publish(&g_pub_adc, &v_msg, nullptr);

			// 2. Publish the string name to /detected_hat
			const char* name;
			if (ap.hat_id == 1)      name = "HAT1";
			else if (ap.hat_id == 2) name = "HAT2";
			else                     name = "Disconnected";

			strcpy(g_hat_msg.data.data, name);
			g_hat_msg.data.size = strlen(g_hat_msg.data.data);

			rcl_publish(&g_pub_hat_name, &g_hat_msg, nullptr);
		}
	}
}

bool MicroRosThread::try_connect_and_setup()
{
	rmw_uros_set_custom_transport(
		true, NULL,
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

	rclc_node_init_default(&g_node, "cubemx_node", "", &g_support);

	// Initializing Publishers
	rclc_publisher_init_default(
		&g_pub_beat, &g_node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), "beat");

	rclc_publisher_init_default(
		&g_pub_subs, &g_node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "subs");

	// NEW: Initialize /adc publisher
	rclc_publisher_init_default(
		&g_pub_adc, &g_node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), "adc");

	//hat pub
	rclc_publisher_init_default(
		&g_pub_hat_name, &g_node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String), "detected_hat");

	// Pre-allocate string memory (Micro-ROS needs this)
	g_hat_msg.data.data = (char*) malloc(20 * sizeof(char));
	g_hat_msg.data.capacity = 20;

	// Initializing Subscriber
	rclc_subscription_init_default(
		&g_sub_test, &g_node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "test_sub");

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
		rcl_publisher_fini(&g_pub_adc, &g_node); // <--- NEW: Clean up ADC

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
