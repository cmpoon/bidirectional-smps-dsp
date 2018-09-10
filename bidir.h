/*
 * bidir.h
 *
 *  Created on: 10 Jun 2018
 *      Author: Chung
 */

#ifndef BIDIR_H_
#define BIDIR_H_

/**
 * Include headers
 */

#define FILTERED_INPUTS

#define GPIO_DC_Relay GPIO_Number_33
//#define GPIO_DC_Relay GPIO_Number_17 // both are connected but 17 has failed on board 1
#define GPIO_AC_Relay GPIO_Number_32
#define GPIO_Precharge GPIO_Number_12
#define GPIO_DBG_ADC_Read GPIO_Number_28

#include "DSP28x_Project.h" // Device Headerfile and Examples Include File

//The following are under C2000Ware_1_00_04_00\device_support\f2802x\common\include
#include "adc.h"
#include "clk.h"
#include "flash.h"
#include "gpio.h"
#include "pie.h"
#include "pll.h"
#include "pwm.h"
#include "timer.h"
#include "wdog.h"

//See http://www.ti.com/lit/sw/sprc990/sprc990.pdf for instruction for inclusion in Boot Room
#define MATH_TYPE   IQ_MATH
#define GLOBAL_Q    19 //Do not use = 15, 20, 24, 29 if ROM IQmath not configured properly
#include "IQmathLib.h"
#define INTtoIQ(A)  ((long) (A) << (GLOBAL_Q))
#define IQtoINT(A)  ((long) (A) >> (GLOBAL_Q))
#define PI          3.1415926535
#define SQRT2       1.41421356

//Control libs
//This one includes all the solar functions for soft PLL.
#include "SPLL_1ph_SOGI_IQ.h"

#include "stkov.h"
#include "SFRA_IQ_Include.h"

// Prototype statements for functions found within this file.

void InitGPIOs(void);
void InitADCs(void);
void InitEPwms(void);
void InitSFRA(void);
void InitControllers(void);

// To run in ISR
inline void updatePwmDuty(void);

interrupt void adc_isr(void);
interrupt void epwm1_timer_isr(void);
//interrupt void epwm2_timer_isr(void);
interrupt void cpu_timer1_isr(void);
interrupt void wakeint_isr(void);
interrupt void stkov_isr(void);

//void sinewavegen(void);

void current_controller();
void voltage_controller();

// Global variables
uint16_t EPwm1_DB_Direction;
uint16_t EPwm2_DB_Direction;

// Configure the period for each timer
#define EPWM_TIMER_TBPRD  1500  // Period register
// Maximum Dead Band values
#define EPWM_MIN_DB   100 //was 30 hootthru q3q4
#define EPWM_MAX_DB   200

// To keep track of which way the Dead Band is moving
#define DB_UP   1
#define DB_DOWN 0

ADC_Handle myAdc;
CLK_Handle myClk;
FLASH_Handle myFlash;
GPIO_Handle myGpio;
PIE_Handle myPie;
PWM_Handle myPwm1, myPwm2;
TIMER_Handle myTimer1;

// Global variables used in this example:
unsigned long system_uptime = 0;

#define VDC_REF 70.0
#define VRMS_REF 40.0

// Store numbers
volatile _iq Vdcref = _IQ(VDC_REF);
volatile _iq Vacmag = _IQ(VRMS_REF * SQRT2);
volatile _iq Vacref = _IQ(VRMS_REF);

volatile _iq VdcADC;
volatile _iq VacADC;
volatile _iq IacADC;

volatile _iq Vdc;
volatile _iq Vac;
volatile _iq Iac;

volatile _iq pwm_duty = _IQ(0.0);
volatile int pwm_out = 0;

volatile int k = 0;
volatile int state = 0;
volatile int rawADCVals[5]; // Range: 0 to 4095 (2^12 -1)

/*********** Calibration **********************/

// V = SCALE (x + OFFSET)
//No filter calibration
//#define VAC_ADC_OFFSET  25
//#define VAC_ADC_SCALE 0.0641854484 // sqrt2 * 0.049163322
//
//#define VDC_ADC_OFFSET  127
//#define VDC_ADC_SCALE   0.045342732
//
////offset is negated
//#define IAC_ADC_OFFSET  2068
//#define IAC_ADC_SCALE   0.0201465201
//With filter calibration
#define VAC_ADC_OFFSET  -25
#define VAC_ADC_SCALE 0.0548327791 //single point cal (116.18300/2)/68*sqrt2 * 0.049163322

#define VDC_ADC_OFFSET 32
#define VDC_ADC_SCALE  0.040520281

//offset is negated
#define IAC_ADC_OFFSET  2050
//#define IAC_ADC_SCALE   0.0367385073 //Board 1
#define IAC_ADC_SCALE   0.04676005949

/*********** Variables for the PLL ************/

_iq theta_hat = _IQ(0.0); // The phase of Vac
//_iq PLL_PHASE_OFFSET = _IQ(0 * 2 * PI); // ticks delay out of 400 per power cycle; NOW Brd 1:  66/400  WAS Brd 1: 42/400

//PLL
SPLL_1ph_SOGI_IQ spll;

// loop filter coefficients for 20kHz
#define A1_LPF SPLL_SOGI_Q(-1.0)

//Tsettle = 0.04ms; zeta=0.07; err = 0.05
//#define B0_LPF SPLL_SOGI_Q(166.9743);
//#define B1_LPF  SPLL_SOGI_Q(-166.266);

//Tsettle = 0.03ms; zeta=0.07; err = 0.05
#define B0_LPF SPLL_SOGI_Q(222.789834)
#define B1_LPF SPLL_SOGI_Q(-221.5307727)

#define GRID_FREQ 50
#define ISR_FREQUENCY 20000.0

/**********************************************/

/**** Variables for the Voltage Controller ****/

_iq Iacref = _IQ(0); // The output to the voltage controller is the reference signal to the current controller

_iq V_error = _IQ(0);
_iq Idcref = _IQ(0);

/**********************************************/

/**** Variables for the Current Controller ****/
// Use PR controller structure
#define pr
_iq I_error = _IQ(0);
_iq Vff = _IQ(0);
_iq Vhr = _IQ(0);

//Hinf
#ifdef hinf

#define ICTRL_DEG 8
#include "CNTL_8P8Z_IQ.h"

CNTL_8P8Z_IQ_COEFFS cntl_ictrl_coeffs1;
CNTL_8P8Z_IQ_VARS cntl_ictrl_vars1;
#endif

#ifdef pr

#include "CNTL_PI_IQ.h"
#include "CNTL_2P2Z_IQ.h"

// PI controller for Voltage Controller (these coefficients are calculated with a 20 kHz sampling rate)
//CNTL_PI_IQ cntl_vctrl_pi;
CNTL_2P2Z_IQ_COEFFS cntl_vctrl_coeffs1;
CNTL_2P2Z_IQ_VARS cntl_vctrl_vars1;

CNTL_PI_IQ cntl_ictrl_pi;
CNTL_2P2Z_IQ_COEFFS cntl_pr1_coeffs1;
CNTL_2P2Z_IQ_VARS cntl_pr1_vars1;
CNTL_2P2Z_IQ_COEFFS cntl_pr3_coeffs1;
CNTL_2P2Z_IQ_VARS cntl_pr3_vars1;
CNTL_2P2Z_IQ_COEFFS cntl_pr5_coeffs1;
CNTL_2P2Z_IQ_VARS cntl_pr5_vars1;
CNTL_2P2Z_IQ_COEFFS cntl_pr7_coeffs1;
CNTL_2P2Z_IQ_VARS cntl_pr7_vars1;

#endif

// Uncomment to use software frequency response analyser
//#define SFRA_ENABLE

#ifdef SFRA_ENABLE
#define SFRA_ISR_FREQ 20000
#define SFRA_FREQ_START 50
#define SFRA_FREQ_LENGTH 120
//SFRA step Multiply = 10^(1/No of steps per decade(40))
#define SFREQ_STEP_MULTIPLY (float)1.059253

SFRA_IQ SFRA1;

//extern to access tables in ROM
extern long IQsinTable[];

int32 Plant_MagVect[SFRA_FREQ_LENGTH];
int32 Plant_PhaseVect[SFRA_FREQ_LENGTH];
int32 OL_MagVect[SFRA_FREQ_LENGTH];
int32 OL_PhaseVect[SFRA_FREQ_LENGTH];
float32 FreqVect[SFRA_FREQ_LENGTH];
#endif
/**********************************************/

/** Helper for system status and JTAG logging **/
volatile int adc_active = 0;
volatile int pwm_en = 0;
volatile int ctrl_en = 0;
volatile int stop_error = 0;
volatile int graceful_shutdown = 0;

#define LOG_LENGTH 50
volatile Uint16 log_flag = 1;
int32 log[LOG_LENGTH] = { 0 };
int32 log2[LOG_LENGTH] = { 0 };

/**
 * Define external locations
 */

// Functions that will be run from RAM need to be assigned to
// a different section.  This section will then be mapped using
// the linker cmd file.
//#pragma CODE_SECTION(adc_isr, "ramfuncs");
#pragma CODE_SECTION(adc_isr ,"Func1")
#pragma CODE_SECTION(voltage_controller ,"Func1")
#pragma CODE_SECTION(current_controller ,"Func1")
#pragma CODE_SECTION(updatePwmDuty ,"Func1")

extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadEnd;
extern Uint16 RamfuncsRunStart;
extern Uint16 RamfuncsLoadSize;
extern Uint16 IQmath_loadstart;
extern Uint16 IQmath_loadsize;
extern Uint16 IQmath_runstart;
extern Uint16 Func1_loadstart;
extern Uint16 Func1_loadsize;
extern Uint16 Func1_runstart;

// Stack overflow detection
extern unsigned int HWI_STKBOTTOM;
extern unsigned int HWI_STKTOP;
unsigned long ADDR_STKBOTTOM;
unsigned long ADDR_STKTOP;
unsigned int stkov_error;
#define margin 45

// Use flash
// #define FLASH 1

#endif /* BIDIR_H_ */
