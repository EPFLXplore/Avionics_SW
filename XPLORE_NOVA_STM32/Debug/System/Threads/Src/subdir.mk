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
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../System/Threads/Inc -I../System/Core/Inc -I../System/Threads/Src -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-System-2f-Threads-2f-Src

clean-System-2f-Threads-2f-Src:
	-$(RM) ./System/Threads/Src/HeartBeat.cyclo ./System/Threads/Src/HeartBeat.d ./System/Threads/Src/HeartBeat.o ./System/Threads/Src/HeartBeat.su ./System/Threads/Src/LedsThread.cyclo ./System/Threads/Src/LedsThread.d ./System/Threads/Src/LedsThread.o ./System/Threads/Src/LedsThread.su ./System/Threads/Src/MassThread.cyclo ./System/Threads/Src/MassThread.d ./System/Threads/Src/MassThread.o ./System/Threads/Src/MassThread.su ./System/Threads/Src/MicroRosThread.cyclo ./System/Threads/Src/MicroRosThread.d ./System/Threads/Src/MicroRosThread.o ./System/Threads/Src/MicroRosThread.su ./System/Threads/Src/TestTask.cyclo ./System/Threads/Src/TestTask.d ./System/Threads/Src/TestTask.o ./System/Threads/Src/TestTask.su ./System/Threads/Src/Thread.cyclo ./System/Threads/Src/Thread.d ./System/Threads/Src/Thread.o ./System/Threads/Src/Thread.su ./System/Threads/Src/servoThreadnew.cyclo ./System/Threads/Src/servoThreadnew.d ./System/Threads/Src/servoThreadnew.o ./System/Threads/Src/servoThreadnew.su

.PHONY: clean-System-2f-Threads-2f-Src

