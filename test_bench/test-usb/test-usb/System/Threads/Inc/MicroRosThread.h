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

class MicroRosThread : public Thread {
public:
    explicit MicroRosThread(QueueHandle_t toRosQueue);

protected:
    virtual void init() override;
    virtual void loop() override;

private:
    QueueHandle_t queue_to_ros;
    bool          initialized = false;
    bool try_connect_and_setup();
};
