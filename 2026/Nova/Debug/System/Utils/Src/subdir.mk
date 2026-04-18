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
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G483xx -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I"C:/Users/LENOVOµ/OneDrive/Documents/EPFL Xplore/ERC Electronics 25-26/Avionics_SW/2026/Nova/System/Threads/Inc" -I"C:/Users/LENOVOµ/OneDrive/Documents/EPFL Xplore/ERC Electronics 25-26/Avionics_SW/2026/Nova/System/Utils/Inc" -I"C:/Users/LENOVOµ/OneDrive/Documents/EPFL Xplore/ERC Electronics 25-26/Avionics_SW/2026/Nova/System/Core/Inc" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-System-2f-Utils-2f-Src

clean-System-2f-Utils-2f-Src:
	-$(RM) ./System/Utils/Src/Operators.cyclo ./System/Utils/Src/Operators.d ./System/Utils/Src/Operators.o ./System/Utils/Src/Operators.su

.PHONY: clean-System-2f-Utils-2f-Src

