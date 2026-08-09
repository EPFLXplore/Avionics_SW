################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../System/Threads/Src/HeartBeatThread.cpp \
../System/Threads/Src/LedsThread.cpp \
../System/Threads/Src/MassThread.cpp \
../System/Threads/Src/SerialThread.cpp \
../System/Threads/Src/ServoThread.cpp \
../System/Threads/Src/Thread.cpp \
../System/Threads/Src/pHMeterThread.cpp 

OBJS += \
./System/Threads/Src/HeartBeatThread.o \
./System/Threads/Src/LedsThread.o \
./System/Threads/Src/MassThread.o \
./System/Threads/Src/SerialThread.o \
./System/Threads/Src/ServoThread.o \
./System/Threads/Src/Thread.o \
./System/Threads/Src/pHMeterThread.o 

CPP_DEPS += \
./System/Threads/Src/HeartBeatThread.d \
./System/Threads/Src/LedsThread.d \
./System/Threads/Src/MassThread.d \
./System/Threads/Src/SerialThread.d \
./System/Threads/Src/ServoThread.d \
./System/Threads/Src/Thread.d \
./System/Threads/Src/pHMeterThread.d 


# Each subdirectory must supply rules for building sources it contributes
System/Threads/Src/%.o System/Threads/Src/%.su System/Threads/Src/%.cyclo: ../System/Threads/Src/%.cpp System/Threads/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++17 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G483xx -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../USB_Device/App -I../USB_Device/Target -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Threads/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Core/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Config/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Comms/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/ERC_SE_CustomMessages/include" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-System-2f-Threads-2f-Src

clean-System-2f-Threads-2f-Src:
	-$(RM) ./System/Threads/Src/HeartBeatThread.cyclo ./System/Threads/Src/HeartBeatThread.d ./System/Threads/Src/HeartBeatThread.o ./System/Threads/Src/HeartBeatThread.su ./System/Threads/Src/LedsThread.cyclo ./System/Threads/Src/LedsThread.d ./System/Threads/Src/LedsThread.o ./System/Threads/Src/LedsThread.su ./System/Threads/Src/MassThread.cyclo ./System/Threads/Src/MassThread.d ./System/Threads/Src/MassThread.o ./System/Threads/Src/MassThread.su ./System/Threads/Src/SerialThread.cyclo ./System/Threads/Src/SerialThread.d ./System/Threads/Src/SerialThread.o ./System/Threads/Src/SerialThread.su ./System/Threads/Src/ServoThread.cyclo ./System/Threads/Src/ServoThread.d ./System/Threads/Src/ServoThread.o ./System/Threads/Src/ServoThread.su ./System/Threads/Src/Thread.cyclo ./System/Threads/Src/Thread.d ./System/Threads/Src/Thread.o ./System/Threads/Src/Thread.su ./System/Threads/Src/pHMeterThread.cyclo ./System/Threads/Src/pHMeterThread.d ./System/Threads/Src/pHMeterThread.o ./System/Threads/Src/pHMeterThread.su

.PHONY: clean-System-2f-Threads-2f-Src

