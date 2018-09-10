# Birdirectional AC/DC Power Supply DSP Code
### Version 4a Poon
## Main firmware has test codes embedded - please take care to configure the states or I/Os operations as needed
------------------

**2018 June**

Note filter Z coefficients based on S domain Simulink values at 20kHz. See models/ for details.
 
See bidir.h header file for details and config

DSP Code to run on C2000 F28027 for PCBv1

## Compilation Header Files

 - ${CG_TOOL_ROOT}/include
 - ${INSTALLROOT_F2802x}/common/include
 - ${INSTALLROOT_F2802x}/headers/include
 - ${INSTALLROOT_IQMATH}/include
 - ${PROJECT_LOC}
 - ${CTRLSUITE_ROOT}/libs/dsp/FixedPointLib/v1_20_00_00/include
 - ${CTRLSUITE_ROOT}/libs/app_libs/solar/v1.2/IQ/include
 - ${CTRLSUITE_ROOT}/libs/app_libs/SFRA/v1_20_00_00/IQ/include
 

## Linkling Libraries
To compile, ensure the following libs are configured in the linker in --priority mode:
 
 - libc.a
 - rts2800_ml.lib
 - 2802x_IQmath_BootROMSymbols.lib
 - IQmath.lib
 - Solar_Lib_IQ.lib
 - SFRA_IQ_Lib.lib
 - c28x_fixedpoint_dsp_library.lib
 
 These can be found in the C2000Ware package, or the controlSuite `device_support` folder:
 
 - ${CG_TOOL_ROOT}/lib
 - ${CTRLSUITE_ROOT}/libs/dsp/FixedPointLib/v1_20_00_00/lib
 - ${C2000_LIBROOT}\boot_rom\f2802x\v2_0\rom_symbol_libs\IQmath
 - ${INSTALLROOT_IQMATH}/lib
 - ${INSTALLROOT_F2802x}
 - ${CTRLSUITE_ROOT}/libs/app_libs/solar/v1.2/IQ/lib
 - ${CTRLSUITE_ROOT}/libs/app_libs/SFRA/v1_20_00_00/IQ/lib
 - ${INSTALLROOT_F2802x}/headers/cmd
