################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../LC\ files/Src/load_cell.cpp 

OBJS += \
./LC\ files/Src/load_cell.o 

CPP_DEPS += \
./LC\ files/Src/load_cell.d 


# Each subdirectory must supply rules for building sources it contributes
LC\ files/Src/load_cell.o: ../LC\ files/Src/load_cell.cpp LC\ files/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I"/Users/brthadm/STM32CubeIDE/workspace_1.19.0/load_cell/LC files/Inc" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32H7xx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"LC files/Src/load_cell.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-LC-20-files-2f-Src

clean-LC-20-files-2f-Src:
	-$(RM) ./LC\ files/Src/load_cell.cyclo ./LC\ files/Src/load_cell.d ./LC\ files/Src/load_cell.o ./LC\ files/Src/load_cell.su

.PHONY: clean-LC-20-files-2f-Src

