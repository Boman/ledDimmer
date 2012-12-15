################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/uart/subdir.mk
-include src/irmp/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
ledDimmerFirmware.lss \

FLASH_IMAGE += \
ledDimmerFirmware.hex \

EEPROM_IMAGE += \
ledDimmerFirmware.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: ledDimmerFirmware.elf secondary-outputs

# Tool invocations
ledDimmerFirmware.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,ledDimmerFirmware.map -mmcu=atmega1284p -o "ledDimmerFirmware.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

ledDimmerFirmware.lss: ledDimmerFirmware.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S ledDimmerFirmware.elf  >"ledDimmerFirmware.lss"
	@echo 'Finished building: $@'
	@echo ' '

ledDimmerFirmware.hex: ledDimmerFirmware.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex ledDimmerFirmware.elf  "ledDimmerFirmware.hex"
	@echo 'Finished building: $@'
	@echo ' '

ledDimmerFirmware.eep: ledDimmerFirmware.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex ledDimmerFirmware.elf  "ledDimmerFirmware.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: ledDimmerFirmware.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega1284p ledDimmerFirmware.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(EEPROM_IMAGE)$(FLASH_IMAGE)$(ELFS)$(LSS)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS) ledDimmerFirmware.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
