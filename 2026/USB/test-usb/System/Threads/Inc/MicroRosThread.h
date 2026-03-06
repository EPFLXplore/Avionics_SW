/*
 * MicroROSThread.h
 *
 *  Created on: Nov 29, 2025
 *      Author: pedro
 */

#pragma once

#include "Thread.h"
#include "packets.h"

extern "C" {
#include "FreeRTOS.h"
#include "queue.h"
}


extern "C" {
#include "usb_device.h"
#include "usbd_core.h"
#include <rcl/rcl.h>
#include <rcl/time.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/float32.h>
}

struct ThreadsRegistry;


class MicroRosThread : public Thread {
public:
    explicit MicroRosThread(ThreadsRegistry* registry);

protected:
    virtual void init() override;
    virtual void loop() override;

private:
    ThreadsRegistry* _reg;
    bool initialized = false;

    void TestCallback(const std_msgs__msg__Int32 * msg);
    void ServoCallback(const std_msgs__msg__Int32 * msg);

    void updateSubs();
    void updatePubs();

    bool try_connect_and_setup();
};
