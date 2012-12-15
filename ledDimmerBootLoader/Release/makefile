################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/uart/subdir.mk
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
ledDimmerBootLoader.lss \

FLASH_IMAGE += \
ledDimmerBootLoader.hex \

EEPROM_IMAGE += \
ledDimmerBootLoader.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: ledDimmerBootLoader.elf secondary-outputs

# Tool invocations
ledDimmerBootLoader.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,ledDimmerBootLoader.map -Ttext=0x1F000 -mmcu=atmega1284p -o "ledDimmerBootLoader.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

ledDimmerBootLoader.lss: ledDimmerBootLoader.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S ledDimmerBootLoader.elf  >"ledDimmerBootLoader.lss"
	@echo 'Finished building: $@'
	@echo ' '

ledDimmerBootLoader.hex: ledDimmerBootLoader.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex ledDimmerBootLoader.elf  "ledDimmerBootLoader.hex"
	@echo 'Finished building: $@'
	@echo ' '

ledDimmerBootLoader.eep: ledDimmerBootLoader.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex ledDimmerBootLoader.elf  "ledDimmerBootLoader.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: ledDimmerBootLoader.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega1284p ledDimmerBootLoader.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(EEPROM_IMAGE)$(FLASH_IMAGE)$(ELFS)$(LSS)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS) ledDimmerBootLoader.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
