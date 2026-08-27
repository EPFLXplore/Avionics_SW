################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../System/Core/Src/ADS1114.cpp \
../System/Core/Src/Bridge.cpp \
../System/Core/Src/HX711.cpp \
../System/Core/Src/LEDStrip.cpp \
../System/Core/Src/PWMDriver.cpp \
../System/Core/Src/System.cpp \
../System/Core/Src/WS2812Driver.cpp 

OBJS += \
./System/Core/Src/ADS1114.o \
./System/Core/Src/Bridge.o \
./System/Core/Src/HX711.o \
./System/Core/Src/LEDStrip.o \
./System/Core/Src/PWMDriver.o \
./System/Core/Src/System.o \
./System/Core/Src/WS2812Driver.o 

CPP_DEPS += \
./System/Core/Src/ADS1114.d \
./System/Core/Src/Bridge.d \
./System/Core/Src/HX711.d \
./System/Core/Src/LEDStrip.d \
./System/Core/Src/PWMDriver.d \
./System/Core/Src/System.d \
./System/Core/Src/WS2812Driver.d 


# Each subdirectory must supply rules for building sources it contributes
System/Core/Src/%.o System/Core/Src/%.su System/Core/Src/%.cyclo: ../System/Core/Src/%.cpp System/Core/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++17 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G483xx -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../USB_Device/App -I../USB_Device/Target -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Threads/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Core/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Config/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/System/Comms/Inc" -I"/home/pedro/Pedro/Xplore/Gits/Avionics_SW/2026/Nova/Avionics_Common/include" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-System-2f-Core-2f-Src

clean-System-2f-Core-2f-Src:
	-$(RM) ./System/Core/Src/ADS1114.cyclo ./System/Core/Src/ADS1114.d ./System/Core/Src/ADS1114.o ./System/Core/Src/ADS1114.su ./System/Core/Src/Bridge.cyclo ./System/Core/Src/Bridge.d ./System/Core/Src/Bridge.o ./System/Core/Src/Bridge.su ./System/Core/Src/HX711.cyclo ./System/Core/Src/HX711.d ./System/Core/Src/HX711.o ./System/Core/Src/HX711.su ./System/Core/Src/LEDStrip.cyclo ./System/Core/Src/LEDStrip.d ./System/Core/Src/LEDStrip.o ./System/Core/Src/LEDStrip.su ./System/Core/Src/PWMDriver.cyclo ./System/Core/Src/PWMDriver.d ./System/Core/Src/PWMDriver.o ./System/Core/Src/PWMDriver.su ./System/Core/Src/System.cyclo ./System/Core/Src/System.d ./System/Core/Src/System.o ./System/Core/Src/System.su ./System/Core/Src/WS2812Driver.cyclo ./System/Core/Src/WS2812Driver.d ./System/Core/Src/WS2812Driver.o ./System/Core/Src/WS2812Driver.su

.PHONY: clean-System-2f-Core-2f-Src

