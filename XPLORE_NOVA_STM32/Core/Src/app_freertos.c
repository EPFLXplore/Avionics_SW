///* USER CODE BEGIN Header */
///**
//  ******************************************************************************
//  * File Name          : app_freertos.c
//  * Description        : Code for freertos applications
//  ******************************************************************************
//  * @attention
//  *
//  * Copyright (c) 2026 STMicroelectronics.
//  * All rights reserved.
//  *
//  * This software is licensed under terms that can be found in the LICENSE file
//  * in the root directory of this software component.
//  * If no LICENSE file comes with this software, it is provided AS-IS.
//  *
//  ******************************************************************************
//  */
///* USER CODE END Header */
//
///* Includes ------------------------------------------------------------------*/
//#include "FreeRTOS.h"
//#include "task.h"
//#include "main.h"
//
///* Private includes ----------------------------------------------------------*/
///* USER CODE BEGIN Includes */
//#include "Interface.h"
//#include "usb_device.h"
//
//#include <stdbool.h>
//#include <std_msgs/msg/int32.h>
//
//#include "rcl/error_handling.h"
//#include "rcl/rcl.h"
//#include "rclc/executor.h"
//#include "rclc/rclc.h"
//#include "rmw_microros/rmw_microros.h"
//#include "rmw_microxrcedds_c/config.h"
//#include "std_msgs/msg/int32.h"
//#include "uxr/client/transport.h"
///* USER CODE END Includes */
//
///* Private typedef -----------------------------------------------------------*/
///* USER CODE BEGIN PTD */
//
///* USER CODE END PTD */
//
///* Private define ------------------------------------------------------------*/
///* USER CODE BEGIN PD */
//
///* USER CODE END PD */
//
///* Private macro -------------------------------------------------------------*/
///* USER CODE BEGIN PM */
//
///* USER CODE END PM */
//
///* Private variables ---------------------------------------------------------*/
///* USER CODE BEGIN Variables */
//
///* USER CODE END Variables */
//
///* Private function prototypes -----------------------------------------------*/
///* USER CODE BEGIN FunctionPrototypes */
//
//bool cubemx_transport_open(struct uxrCustomTransport * transport);
//bool cubemx_transport_close(struct uxrCustomTransport * transport);
//size_t cubemx_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * err);
//size_t cubemx_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err);
//
//void * microros_allocate(size_t size, void * state);
//void microros_deallocate(void * pointer, void * state);
//void * microros_reallocate(void * pointer, size_t size, void * state);
//void * microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state);
//
//void InitSystem(void);
//
///* USER CODE END FunctionPrototypes */
//
///* Private application code --------------------------------------------------*/
///* USER CODE BEGIN Application */
//void InitSystem(void){
//	MX_USB_Device_Init();
//	InterfaceSystemInit();
//}
///* USER CODE END Application */
//
