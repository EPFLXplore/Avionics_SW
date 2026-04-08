/*
 * TestTask.cpp
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#include <MassThread.h>

MassThread::MassThread(const char* name, osPriority priority)
: MessageThread(name, priority)
{
	load_cell_0 = new HX711(HX711_DATA_GPIO_Port, HX711_DATA_Pin, HX711_CLK_GPIO_Port,  HX711_CLK_Pin); //TODO change this pointer thingy to not have two heap allocs
	mass_0 = new MassType;
	if (load_cell_0){
		mass_0->hx = load_cell_0;
	}
	load_cell_1 = new HX711(HX2_DATA_GPIO_Port, HX2_DATA_Pin, HX2_CLK_GPIO_Port,  HX2_CLK_Pin); //TODO change this pointer thingy to not have two heap allocs
		mass_1 = new MassType;
	if (load_cell_1){
		mass_1->hx = load_cell_1;
	}
}

MassThread::~MassThread(){

	if (mass_0){
		if (load_cell_0){
			delete load_cell_0;
			load_cell_0 = nullptr;
		}
		delete mass_0;
		mass_0 = nullptr;
	}

	if (mass_1){
		if (load_cell_1){
			delete load_cell_1;
			load_cell_0 = nullptr;
		}
		delete mass_1;
		mass_1 = nullptr;
	}

}

void MassThread::init(){
	mass_0->hx->begin();
	osDelay(110);
	this->tareScale(mass_0);

	mass_1->hx->begin();
	osDelay(110);
	this->tareScale(mass_1);
}

void MassThread::loop(){
    MassRequest cmd;
    if (this->popCommand(cmd)) {
        if (cmd.tare) {
        	if (cmd.id == 0){
        		this->tareScale(this->mass_0);
        	} else {
        		this->tareScale(this->mass_1);
        	}
        }
    }

    this->updateMass(mass_0);
    this->updateMass(mass_1);
}

void MassThread::updateMass(MassType* device){
	this->update(device);
	MassPacket st;
	st.mass = device->weight;
	if (device == mass_0){
	 	st.id = 0;
	} else  {
	 	st.id = 1;
	}
    pushStatus(st);
}


void MassThread::shift(float *array , int N, float valueIn){  //shifts all array values left and adds valueIn at position N-1
  for(int i = 1; i<N ; i++){
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

void MassThread::update(MassType* device)
{
	if (!device->hx->available()) return;

    volatile int32_t raw = device->hx->read();

    // 1. Shift and average
    this->shift(device->buffer, AVG_SIZE, (float)raw);
    float avg = this->movingAverage(device->buffer, AVG_SIZE);

    // 2. Apply offset AND slope
    // Result = (Current - Zero) * CalibrationFactor
   float val = (avg - device->offset) * device->slope;

   device->weight = val;
}

void MassThread::tareScale(MassType* device) {
	if (!device->hx->available()) return;

	for (uint8_t i = 0; i < AVG_SIZE; ++i) {
	    	device->buffer[i] = 0;
	}

	int64_t sum = 0;
	for (uint8_t i = 0; i < 20; ++i) {
		sum += device->hx->read();
	}

    device->offset = (float)(sum/20);

    for (uint8_t i = 0; i < AVG_SIZE; ++i) {
    	device->buffer[i] = device->offset;
    }
}

/*
void MassThread::sendfloat(float value) {
	snprintf(this->buffer, sizeof(buffer), "Mass value = %.3f\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}

void MassThread::sendint(int32_t value) {
	snprintf(this->buffer, sizeof(buffer), "Mass value = %d\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}
*/


