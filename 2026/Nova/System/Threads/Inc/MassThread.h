/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#pragma once

#include "Thread.h"
#include "MessageThread.h"
#include "HX711.h"
#include "packets.h"

#include <sys/time.h>
#include "cmsis_os2.h"
#include "usbd_cdc_if.h"

#include <stdio.h>

constexpr uint8_t AVG_SIZE = 20;

struct MassType {
	HX711* hx = nullptr;
	float offset = 0.0f;
	float slope = 0.00049191f;
	float weight = 0.0f;
	float buffer[AVG_SIZE];
};

class MassThread : public MessageThread<MassRequest, MassPacket>{ //TODO reput the tare
public:
	MassThread(const char* name, osPriority priority);
	~MassThread();

	void init();

	void updateMass(MassType* device);

	void shift(float *array , int N, float valueIn);

	float movingAverage(const float *arr, uint8_t n);

	void update(MassType* device);

	void tareScale(MassType* device);

	void loop();

private:
	HX711* load_cell_0;
	MassType* mass_0;
	HX711* load_cell_1;
	MassType* mass_1;
	int32_t raw = 0;

	char buffer[64];

	//Tiny helper for now DONT USE WITH MICROROS
	void sendfloat(float value);
	void sendint(int32_t value);
};


