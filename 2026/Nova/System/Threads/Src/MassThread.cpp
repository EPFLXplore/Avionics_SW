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
	load_cell = new HX711(HX711_DATA_GPIO_Port, HX711_DATA_Pin, HX711_CLK_GPIO_Port,  HX711_CLK_Pin); //TODO change this pointer thingy to not have two heap allocs
		mass = new MassType;
		if (load_cell){
			mass->hx = load_cell;
		}
}

MassThread::~MassThread(){
	if (load_cell){
		delete load_cell;
		load_cell = nullptr;
	}
	if (mass){
		delete mass;
		mass = nullptr;
	}
}

void MassThread::init(){
	mass->hx->begin();
	osDelay(110);
	this->tareScale(mass);
}

void MassThread::loop(){
    // --- 1) GESTION DES COMMANDES (MICRO-ROS) ---
    MassRequest cmd;
    // Si MicroRosThread a fait un pushCommand(req), on le récupère ici
    /*if (popCommand(cmd)) {
        if (cmd.tare) {
            this->tareScale(this->mass);
        }
    }*/

    // --- 2) MISE À JOUR PHYSIQUE ---
    this->update(mass);

    // --- 3) ENVOI DU STATUS VERS MICROROS ---
    MassPacket st;
    st.mass = mass->weight;
    st.id = 5;
    // Note: status_code peut être ajouté dans la struct MassPacket si nécessaire
    pushStatus(st);

    osDelay(pdMS_TO_TICKS(100));
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
	for (uint8_t i = 0; i < AVG_SIZE; ++i) {
	    	device->buffer[i] = 0;
	}

	int64_t sum = 0;
	for (uint8_t i = 0; i < 20; ++i) {
		sum += device->hx->read();
		osDelay(10);
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


