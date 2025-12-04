################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../System/Utils/Src/Operators.cpp 

OBJS += \
./System/Utils/Src/Operators.o 

CPP_DEPS += \
./System/Utils/Src/Operators.d 


# Each subdirectory must supply rules for building sources it contributes
System/Utils/Src/%.o System/Utils/Src/%.su System/Utils/Src/%.cyclo: ../System/Utils/Src/%.cpp System/Utils/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/test_bench/test-usb/test-usb/System/Core/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/test_bench/test-usb/test-usb/System/Threads/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/test_bench/test-usb/test-usb/System/Utils/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32H7xx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/test_bench/test-usb/test-usb/micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros/include" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-System-2f-Utils-2f-Src

clean-System-2f-Utils-2f-Src:
	-$(RM) ./System/Utils/Src/Operators.cyclo ./System/Utils/Src/Operators.d ./System/Utils/Src/Operators.o ./System/Utils/Src/Operators.su

.PHONY: clean-System-2f-Utils-2f-Src

