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
    explicit MicroRosThread(ThreadsRegistry* registry, const char* name, osPriority priority, uint32_t stackSize = 12288);

protected:
    virtual void init() override;
    virtual void loop() override;

private:
    ThreadsRegistry* _reg;
    bool initialized = false;

    void updateSubs();
    void updatePubs();

    bool try_connect_and_setup();
};
