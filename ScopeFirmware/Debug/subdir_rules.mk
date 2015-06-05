################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me --include_path="C:/ti/TivaWare_C_Series-2.1.0.12573" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/include" -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA0 --display_error_number --diag_warning=225 --diag_wrap=off --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

tm4c1294ncpdt_startup_ccs.obj: ../tm4c1294ncpdt_startup_ccs.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me --include_path="C:/ti/TivaWare_C_Series-2.1.0.12573" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/include" -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA0 --display_error_number --diag_warning=225 --diag_wrap=off --preproc_with_compile --preproc_dependency="tm4c1294ncpdt_startup_ccs.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

uartstdio.obj: C:/ti/TivaWare_C_Series-2.1.0.12573/utils/uartstdio.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me --include_path="C:/ti/TivaWare_C_Series-2.1.0.12573" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.2/include" -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=TARGET_IS_TM4C129_RA0 --display_error_number --diag_warning=225 --diag_wrap=off --preproc_with_compile --preproc_dependency="uartstdio.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


