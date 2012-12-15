################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/irmp/irmp.c \
../src/irmp/irmpextlog.c 

OBJS += \
./src/irmp/irmp.o \
./src/irmp/irmpextlog.o 

C_DEPS += \
./src/irmp/irmp.d \
./src/irmp/irmpextlog.d 


# Each subdirectory must supply rules for building sources it contributes
src/irmp/%.o: ../src/irmp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -Os -fpack-struct -fshort-enums -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega1284p -DF_CPU=20000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


