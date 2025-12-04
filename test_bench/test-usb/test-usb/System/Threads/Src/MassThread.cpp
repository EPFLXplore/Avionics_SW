/*
 * TestTask.cpp
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#include <MassThread.h>

MassThread::MassThread(QueueHandle_t toRosQueue) : Thread("MassThread", (osPriority)osPriorityNormal5, (uint32_t) 2048), queue_to_ros(toRosQueue){
	load_cell = new HX711(HD_DOUT_GPIO_Port, HD_DOUT_Pin, HD_SCK_GPIO_Port,  HD_SCK_Pin); //TODO change this pointer thingy to not have two heap allocs
	mass = new MassType;
	if (load_cell){
		mass->hx = load_cell;
	}
}

void MassThread::init(){
	mass->hx->begin();
	osDelay(pdMS_TO_TICKS(1));
}

void MassThread::loop(){
	this->update(mass);
	//raw = mass->hx->read();
	//this->sendint(raw);
	if (queue_to_ros != nullptr) {
		SystemMessage msg;
	    msg.type = PacketType::MASS_PACKET;
	    msg.data.mass_packet.id   = 0;     // sensor id if needed
	    msg.data.mass_packet.mass = 333.33;

	    xQueueSend(queue_to_ros, &msg, 0);
	}
	osDelay(pdMS_TO_TICKS(500));
	//this->sendfloat(mass->weight);
}

void MassThread::shift(float *array , int N, float valueIn){  //shifts all array values left and adds valueIn at position N-1
  for(int i = 1; i<N-1 ; i++){
    array[i-1] = array[i];
  }
  array[N-1] = valueIn;
}

float MassThread::movingAverage(const float *arr, uint8_t n) {
  if(n<=0){return 0;}
  float sum = 0.0f;
  for (uint8_t i = 0; i < n; i++) sum += arr[i];
  return sum / n;
}

void MassThread::update(MassType* device) {
    if (!device->hx->available()) return;
    long raw = device->hx->read();
    this->shift(device->buffer, AVG_SIZE, (float) raw);
    float avg = this->movingAverage(device->buffer, AVG_SIZE);
    device->weight = (avg - device->offset) * device->slope;
}

void MassThread::tareScale(MassType* device) {
    device->offset = device->hx->read();
    device->hx->tare();
    for (uint8_t i = 0; i < AVG_SIZE; ++i) {
    	device->buffer[i] = device->offset;
    }
    osDelay(100); //TODO ms or ticks?
}


void MassThread::sendfloat(float value) {
	snprintf(this->buffer, sizeof(buffer), "Mass value = %.3f\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}

void MassThread::sendint(int32_t value) {
	snprintf(this->buffer, sizeof(buffer), "Mass value = %d\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}


