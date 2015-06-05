################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2

ORDERED_OBJS += \
"./main.obj" \
"./tm4c1294ncpdt_startup_ccs.obj" \
"./uartstdio.obj" \
"../tm4c1294ncpdt.cmd" \
$(GEN_CMDS__FLAG) \
-l"libc.a" \
-l"C:/ti/TivaWare_C_Series-2.1.0.12573/driverlib/ccs/Debug/driverlib.lib" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
Experiments.out \

EXE_OUTPUTS__QUOTED += \
"Experiments.out" \

BIN_OUTPUTS += \
Experiments.hex \

BIN_OUTPUTS__QUOTED += \
"Experiments.hex" \


# All Target
all: Experiments.out

dependents:
	-cd C:/ti/TivaWare_C_Series-2.1.0.12573/driverlib/ccs/Debug && $(MAKE) all

# Tool invocations
Experiments.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS) C:/ti/TivaWare_C_Series-2.1.0.12573/driverlib/ccs/Debug/driverlib.lib
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Linker'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA0 --display_error_number --diag_warning=225 --diag_wrap=off -z -m"Experiments.map" --heap_size=0 --stack_size=512 -i"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/lib" --reread_libs --warn_sections --display_error_number --diag_wrap=off --xml_link_info="Experiments_linkInfo.xml" --rom_model -o "Experiments.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '

Experiments.hex: $(EXE_OUTPUTS)
	@echo 'Invoking: ARM Hex Utility'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/bin/armhex"  -o "Experiments.hex" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)$(BIN_OUTPUTS__QUOTED)
	-$(RM) "main.pp" "tm4c1294ncpdt_startup_ccs.pp" "uartstdio.pp" 
	-$(RM) "main.obj" "tm4c1294ncpdt_startup_ccs.obj" "uartstdio.obj" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
C:/ti/TivaWare_C_Series-2.1.0.12573/driverlib/ccs/Debug/driverlib.lib:

-include ../makefile.targets

