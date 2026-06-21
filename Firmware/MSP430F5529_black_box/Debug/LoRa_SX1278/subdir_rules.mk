################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
LoRa_SX1278/%.obj: ../LoRa_SX1278/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/App/CCSTUDIO/setup/ccs/tools/compiler/ti-cgt-msp430_21.6.2.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/App/CCSTUDIO/setup/ccs/ccs_base/msp430/include" --include_path="D:/projects/Flight_Black_Box/Firmware/MSP430F5529_black_box" --include_path="D:/App/CCSTUDIO/setup/ccs/tools/compiler/ti-cgt-msp430_21.6.2.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="LoRa_SX1278/$(basename $(<F)).d_raw" --obj_directory="LoRa_SX1278" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


