#ifndef TIMERS_H
#define TIMERS_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif


extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;   // LED strip 1 — PC6  (LEDS_TIM2_Pin), DMA2_Stream1
extern TIM_HandleTypeDef htim4;   // LED strip 2 — PD12 (LEDS_TIM1_Pin), DMA1_Stream1
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim15;

#ifdef __cplusplus
}
#endif

#endif


