################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../System/Threads/Src/HeartBeat.cpp \
../System/Threads/Src/LedsThread.cpp \
../System/Threads/Src/MassThread.cpp \
../System/Threads/Src/MicroRosThread.cpp \
../System/Threads/Src/TestTask.cpp \
../System/Threads/Src/Thread.cpp \
../System/Threads/Src/servoThreadnew.cpp 

OBJS += \
./System/Threads/Src/HeartBeat.o \
./System/Threads/Src/LedsThread.o \
./System/Threads/Src/MassThread.o \
./System/Threads/Src/MicroRosThread.o \
./System/Threads/Src/TestTask.o \
./System/Threads/Src/Thread.o \
./System/Threads/Src/servoThreadnew.o 

CPP_DEPS += \
./System/Threads/Src/HeartBeat.d \
./System/Threads/Src/LedsThread.d \
./System/Threads/Src/MassThread.d \
./System/Threads/Src/MicroRosThread.d \
./System/Threads/Src/TestTask.d \
./System/Threads/Src/Thread.d \
./System/Threads/Src/servoThreadnew.d 


# Each subdirectory must supply rules for building sources it contributes
System/Threads/Src/%.o System/Threads/Src/%.su System/Threads/Src/%.cyclo: ../System/Threads/Src/%.cpp System/Threads/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G483xx -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/STM32G4xx_HAL_Driver/Inc -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros/include" -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../USB_Device/App -I../USB_Device/Target -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Threads/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Utils/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Core/Inc" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-System-2f-Threads-2f-Src

clean-System-2f-Threads-2f-Src:
	-$(RM) ./System/Threads/Src/HeartBeat.cyclo ./System/Threads/Src/HeartBeat.d ./System/Threads/Src/HeartBeat.o ./System/Threads/Src/HeartBeat.su ./System/Threads/Src/LedsThread.cyclo ./System/Threads/Src/LedsThread.d ./System/Threads/Src/LedsThread.o ./System/Threads/Src/LedsThread.su ./System/Threads/Src/MassThread.cyclo ./System/Threads/Src/MassThread.d ./System/Threads/Src/MassThread.o ./System/Threads/Src/MassThread.su ./System/Threads/Src/MicroRosThread.cyclo ./System/Threads/Src/MicroRosThread.d ./System/Threads/Src/MicroRosThread.o ./System/Threads/Src/MicroRosThread.su ./System/Threads/Src/TestTask.cyclo ./System/Threads/Src/TestTask.d ./System/Threads/Src/TestTask.o ./System/Threads/Src/TestTask.su ./System/Threads/Src/Thread.cyclo ./System/Threads/Src/Thread.d ./System/Threads/Src/Thread.o ./System/Threads/Src/Thread.su ./System/Threads/Src/servoThreadnew.cyclo ./System/Threads/Src/servoThreadnew.d ./System/Threads/Src/servoThreadnew.o ./System/Threads/Src/servoThreadnew.su

.PHONY: clean-System-2f-Threads-2f-Src

