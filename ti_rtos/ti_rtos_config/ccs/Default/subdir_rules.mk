################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
configPkg/: E:/3、相应开发包/CC3200\ SDK/cc3200-sdk/ti_rtos/ti_rtos_config/app.cfg
	@echo 'Building file: $<'
	@echo 'Invoking: XDCtools'
	"F:/ccs v6/xdctools_3_30_03_47_core/xs" --xdcpath="F:/ccs v6/ccsv6/ccs_base;F:/ccs v6/tirtos_simplelink_2_01_00_03/packages;F:/ccs v6/tirtos_simplelink_2_01_00_03/products/bios_6_40_03_39/packages;F:/ccs v6/tirtos_simplelink_2_01_00_03/products/uia_2_00_01_34/packages;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M4 -p ti.platforms.simplelink:CC3200 -r release -c "F:/ccs v6/ccsv6/tools/compiler/arm_5.1.6" "$<"
	@echo 'Finished building: $<'
	@echo ' '


