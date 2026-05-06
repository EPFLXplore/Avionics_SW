################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Adafruit_NeoPixel_STM/Adafruit_NeoPixel_STM.cpp \
../Core/Adafruit_NeoPixel_STM/LEDStrip.cpp 

OBJS += \
./Core/Adafruit_NeoPixel_STM/Adafruit_NeoPixel_STM.o \
./Core/Adafruit_NeoPixel_STM/LEDStrip.o 

CPP_DEPS += \
./Core/Adafruit_NeoPixel_STM/Adafruit_NeoPixel_STM.d \
./Core/Adafruit_NeoPixel_STM/LEDStrip.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Adafruit_NeoPixel_STM/%.o Core/Adafruit_NeoPixel_STM/%.su Core/Adafruit_NeoPixel_STM/%.cyclo: ../Core/Adafruit_NeoPixel_STM/%.cpp Core/Adafruit_NeoPixel_STM/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Adafruit_NeoPixel_STM

clean-Core-2f-Adafruit_NeoPixel_STM:
	-$(RM) ./Core/Adafruit_NeoPixel_STM/Adafruit_NeoPixel_STM.cyclo ./Core/Adafruit_NeoPixel_STM/Adafruit_NeoPixel_STM.d ./Core/Adafruit_NeoPixel_STM/Adafruit_NeoPixel_STM.o ./Core/Adafruit_NeoPixel_STM/Adafruit_NeoPixel_STM.su ./Core/Adafruit_NeoPixel_STM/LEDStrip.cyclo ./Core/Adafruit_NeoPixel_STM/LEDStrip.d ./Core/Adafruit_NeoPixel_STM/LEDStrip.o ./Core/Adafruit_NeoPixel_STM/LEDStrip.su

.PHONY: clean-Core-2f-Adafruit_NeoPixel_STM

