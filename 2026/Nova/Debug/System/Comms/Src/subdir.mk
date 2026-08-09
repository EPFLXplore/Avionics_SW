################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../System/Comms/Src/Transport.cpp 

OBJS += \
./System/Comms/Src/Transport.o 

CPP_DEPS += \
./System/Comms/Src/Transport.d 


# Each subdirectory must supply rules for building sources it contributes
System/Comms/Src/%.o System/Comms/Src/%.su System/Comms/Src/%.cyclo: ../System/Comms/Src/%.cpp System/Comms/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++17 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G483xx -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../USB_Device/App -I../USB_Device/Target -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Threads/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Core/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Config/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Comms/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/ERC_SE_CustomMessages/include" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-System-2f-Comms-2f-Src

clean-System-2f-Comms-2f-Src:
	-$(RM) ./System/Comms/Src/Transport.cyclo ./System/Comms/Src/Transport.d ./System/Comms/Src/Transport.o ./System/Comms/Src/Transport.su

.PHONY: clean-System-2f-Comms-2f-Src

