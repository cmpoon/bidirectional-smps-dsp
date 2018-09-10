/**
 * Bidirectional Converter ACDC Controller V4a Release Candidate 1
 * Chung Poon 2018 June
 * Note filter Z coefficients based on S domain Simulink values at 20kHz
 * See header file for details and config
 *
 */

#include "bidir.h"

/**
 * Main entry point
 */


void main(void)

{
    CPU_Handle myCpu;

    PLL_Handle myPll;
    WDOG_Handle myWDog;
    // Initialize all the handles needed for this application
    myAdc = ADC_init((void *) ADC_BASE_ADDR, sizeof(ADC_Obj));
    myClk = CLK_init((void *) CLK_BASE_ADDR, sizeof(CLK_Obj));
    myCpu = CPU_init((void *) NULL, sizeof(CPU_Obj));
    myFlash = FLASH_init((void *) FLASH_BASE_ADDR, sizeof(FLASH_Obj));
    myGpio = GPIO_init((void *) GPIO_BASE_ADDR, sizeof(GPIO_Obj));
    myPie = PIE_init((void *) PIE_BASE_ADDR, sizeof(PIE_Obj));
    myPll = PLL_init((void *) PLL_BASE_ADDR, sizeof(PLL_Obj));
    myPwm1 = PWM_init((void *) PWM_ePWM1_BASE_ADDR, sizeof(PWM_Obj));
    myPwm2 = PWM_init((void *) PWM_ePWM2_BASE_ADDR, sizeof(PWM_Obj));
    myWDog = WDOG_init((void *) WDOG_BASE_ADDR, sizeof(WDOG_Obj));
    myTimer1 = TIMER_init((void *) TIMER1_BASE_ADDR, sizeof(TIMER_Obj));

    // Perform basic system initialization
    WDOG_disable(myWDog);
    CLK_enableAdcClock(myClk);
    (*Device_cal )();
    //CLK_disableAdcClock(myClk);

    //Select the internal oscillator 1 as the clock source
    CLK_setOscSrc(myClk, CLK_OscSrc_Internal);

    // Setup the PLL for x10 /2 which will yield 50Mhz = 10Mhz * 10 / 2
    PLL_setup(myPll, PLL_Multiplier_12, PLL_DivideSelect_ClkIn_by_1);

    // Disable the PIE and all interrupts during config
    PIE_disable(myPie);
    PIE_disableAllInts(myPie);
    CPU_disableGlobalInts(myCpu);
    CPU_clearIntFlags(myCpu);

    // If running from flash, copy RAM only functions to RAM
    memcpy(&Func1_runstart, &Func1_loadstart, (Uint32) &Func1_loadsize);
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t) &RamfuncsLoadSize);
    memcpy(&IQmath_runstart, &IQmath_loadstart, (Uint32) &IQmath_loadsize);
#ifdef _FLASH
    FLASH_setup(myFlash);
#endif

    // Setup a debug vector table and enable the PIE
    PIE_setDebugIntVectorTable(myPie);
    PIE_enable(myPie);

    //INTERRUPTS
    // Register interrupt handlers in the PIE vector table
    //PIE_registerPieIntHandler(myPie, PIE_GroupNumber_3, PIE_SubGroupNumber_1,
    //                         (intVec_t)&epwm1_isr);
    PIE_registerPieIntHandler(myPie, PIE_GroupNumber_10, PIE_SubGroupNumber_1,
                              (intVec_t) & adc_isr);
    PIE_registerPieIntHandler(myPie, PIE_GroupNumber_1, PIE_SubGroupNumber_8,
                              (intVec_t) & wakeint_isr);
    PIE_registerSystemIntHandler(myPie, PIE_SystemInterrupts_TINT1,
                                 (intVec_t) & cpu_timer1_isr);
    PIE_registerSystemIntHandler(myPie, PIE_SystemInterrupts_RTOSINT,
                                 (intVec_t) & stkov_isr);

    //Init Stack overflow monitoring
    ADDR_STKBOTTOM = (unsigned long) &HWI_STKBOTTOM;
    ADDR_STKTOP = (unsigned long) &HWI_STKTOP;
    stkov_error = STKOV_initSystemStack(ADDR_STKBOTTOM, ADDR_STKTOP, margin);

    InitGPIOs();
    // Initialize the ADC
    ADC_enableBandGap(myAdc);
    ADC_enableRefBuffers(myAdc);
    ADC_setVoltRefSrc(myAdc, ADC_VoltageRefSrc_Int);
    ADC_powerUp(myAdc);
    ADC_enable(myAdc);

    // Set up timer
    TIMER_stop(myTimer1);
    TIMER_setPeriod(myTimer1, 100 * 1000000);       //us

    TIMER_setPreScaler(myTimer1, 0);
    TIMER_reload(myTimer1);
    TIMER_setEmulationMode(myTimer1,
                           TIMER_EmulationMode_StopAfterNextDecrement);
    TIMER_enableInt(myTimer1);
    TIMER_start(myTimer1);

    CPU_enableInt(myCpu, CPU_IntNumber_13);

    // Enable ADCINT1 in PIE
    PIE_enableAdcInt(myPie, ADC_IntNumber_1);

    // Enable CPU Interrupt 10
    CPU_enableInt(myCpu, CPU_IntNumber_10);

    InitADCs();
    InitEPwms();

    //Watchdog Interrupt
    WDOG_enableInt(myWDog);
    // Enable WAKEINT in the PIE: Group 1 interrupt 8
    // Enable INT1 which is connected to WAKEINT
    PIE_enableInt(myPie, PIE_GroupNumber_1, PIE_InterruptSource_WAKE);
    CPU_enableInt(myCpu, CPU_IntNumber_1);

    // Enable CPU INT3 which is connected to EPWM1-3 INT:
    CPU_enableInt(myCpu, CPU_IntNumber_3);

    // Enable EPWM INTn in the PIE: Group 3 interrupt 1-3
    PIE_enablePwmInt(myPie, PWM_Number_1);
    //PIE_enablePwmInt(myPie, PWM_Number_2);

    // Enable global Interrupts and higher priority real-time debug events
    CPU_enableGlobalInts(myCpu);
    CPU_enableDebugInt(myCpu);

    WDOG_setPreScaler(myWDog, WDOG_PreScaler_OscClk_by_512_by_64);

    // Reset the watchdog counter
    WDOG_clearCounter(myWDog);

    // Enable the watchdog
    WDOG_enable(myWDog);

    InitControllers();

    //Clear log

    uint16_t i = 0;
    for (i = 0; i < LOG_LENGTH; i++)
    {
        log[i] = (long) 0;
        log2[i] = (long) 0;
    }

    //Power on self test state machines
    for (;;)
    {

        WDOG_clearCounter(myWDog);

        switch (state)
        {
        //Run all disconnected
        case 0:
        default:
            //State 0 - Startup/Shutdown
            //DEFAULT STATE

            //Precharge
            GPIO_setLow(myGpio, GPIO_Precharge);

            //AC Contactor
            GPIO_setLow(myGpio, GPIO_AC_Relay);

            //DC Contactor
            GPIO_setLow(myGpio, GPIO_DC_Relay);

            pwm_en = 0;
            ctrl_en = 0;

            if (adc_active == 1 && stop_error == 0)
            {
                state = 1;
            }
            break;

        case 1:
            //State 1: PRECHARGE
            //DC Contactor
            GPIO_setLow(myGpio, GPIO_DC_Relay);
            //Test <640 only - use setHigh above to connect DC only

            //AC Contactor
            GPIO_setLow(myGpio, GPIO_AC_Relay);

            //Precharge
            GPIO_setHigh(myGpio, GPIO_Precharge);
            //Test <640 only - use setLow above to discon AC

            pwm_en = 0;
            ctrl_en = 0;

            if (Vdc > _IQ(75))
            {
                stop_error = 3; // 3-DC Overvoltage
            }
            else if (Vdc > _IQ(12))
            {
                state = 2;
            }

            if (stop_error > 0)
            {
                state = 0;
            }

            Iacref = _IQ(0.0);
            break;

        case 2:
            //DC Contactor
            GPIO_setHigh(myGpio, GPIO_DC_Relay);

            //AC Contactor
            GPIO_setHigh(myGpio, GPIO_AC_Relay);
            //Test <640 only - use setLow above to discon AC

            //Precharge
            GPIO_setLow(myGpio, GPIO_Precharge);

            if (GPIO_getData(myGpio, GPIO_DC_Relay) == 0)
            {
                stop_error = 12; // 12 - Relay signal jammed
            }

            if (stop_error == 0)
            {

                pwm_en = 1;
                ctrl_en = 1;

                // Vdc check
                if (Vdc > _IQ(75.0))
                {
                    state = 0;
                    stop_error = 3; // 3- DC Overvoltage
                }
                else if (Vdc < _IQ(8.0))
                { //Note the hysteresis
                    state = 1;
                }

                //Vac Check
                if (Vac > _IQ(61.0))
                {
                    state = 0;
                    stop_error = 4; // 4 - AC Overvoltage
                }

                //Iac check
                if (Iac > _IQ(30) || Iac < _IQ(-30))
                {
                    state = 0;
                    stop_error = 5; // 5 - AC Overcurrent
                }
            }
            else
            {
                state = 0;
            }

            break;
        case 3:

            //DC Contactor
            GPIO_setLow(myGpio, GPIO_DC_Relay);

            //AC Contactor
            GPIO_setLow(myGpio, GPIO_AC_Relay);

            //Precharge
            GPIO_setLow(myGpio, GPIO_Precharge);

            pwm_en = 1;
            ctrl_en = 0;
            graceful_shutdown = 1;
            Iacref = _IQ(0.0);

            if (Vdc < _IQ(1.0))
            {
                //Discharge complete - shutdown
                pwm_en = 0;
                stop_error = 1; // Normal shutdown
            }

            if (stop_error > 0)
            {
                state = 0;
            }

            asm(" NOP");
            break;
        }
#ifdef SFRA_ENABLE
        if (pwm_en && ctrl_en)
        {
            SFRA_IQ_BACKGROUND (&SFRA1);
        }
#endif
    }
}

interrupt void adc_isr(void) // 20kHz SAMPLING RATE
{
    //FLASH_setPowerMode(myFlash, FLASH_PowerMode_PumpAndBankStandby);
    GPIO_setHigh(myGpio, GPIO_DBG_ADC_Read);
    k++;
    rawADCVals[0] = (int) ADC_readResult(myAdc, ADC_ResultNumber_1); //VDC -
    rawADCVals[1] = (int) ADC_readResult(myAdc, ADC_ResultNumber_2); //VDC +
    rawADCVals[2] = (int) ADC_readResult(myAdc, ADC_ResultNumber_3); //Vac -
    rawADCVals[3] = (int) ADC_readResult(myAdc, ADC_ResultNumber_4); //Vac +
    rawADCVals[4] = (int) ADC_readResult(myAdc, ADC_ResultNumber_5); //Iac with Offset

    //This could be out of range for IQ20, maybe try IQ15
    VacADC = _IQ((long) (rawADCVals[1] - rawADCVals[0] - VAC_ADC_OFFSET));
    VdcADC = _IQ((long) (rawADCVals[3] - rawADCVals[2] - VDC_ADC_OFFSET)); //fix for IQ offset
    IacADC = _IQ((rawADCVals[4] - IAC_ADC_OFFSET)); //Offset Applied here to keep within IQ20 range

    // Scaled empirically using calibration results. Adjust in header file. Sense check values below.
    Vac = _IQmpy(VacADC, _IQ( VAC_ADC_SCALE )); // ~ (3.3 / 4095) / (100 / 86100) / 8
    Vdc = _IQmpy(VdcADC, _IQ( VDC_ADC_SCALE )); // ~ (3.3 / 4095) / (100 / 43100) / 8.2 [ADC / Pdiv / AMC Gain]
    Iac = _IQmpy(IacADC, _IQ( IAC_ADC_SCALE )); // ~ 40mV/A ==> Factor of (3.3/4095) / 40e-3

    // SPLL call
    // GridMeas is in IQ19 and is converted to IQ23 for SPLL
    spll.u[0] = _IQtoIQ23(Vac); //deleted VAC/25 pu conversion and IQconv done by lib
    SPLL_1ph_SOGI_IQ_FUNC(&spll);
    Vacmag = _IQmag(_IQ23toIQ(spll.osg_u[0]), _IQ23toIQ(spll.osg_qu[0]));
    theta_hat = _IQ23toIQ(spll.theta[0]) /*- PLL_PHASE_OFFSET*/;
    // VacFilter = _IQ23toIQ(spll.osg_u[0]);

    GPIO_setLow(myGpio, GPIO_DBG_ADC_Read);

    adc_active = 1;

    if (ctrl_en == 1)
    {
#ifdef SFRA_ENABLE
        Vdcref = SFRA_IQ_INJECT(_IQ(VDC_REF));
#endif
        voltage_controller();
        current_controller();

    }

    updatePwmDuty();

    if (log_flag == 1 && k >= ISR_FREQUENCY - LOG_LENGTH)
    {
        log[k % LOG_LENGTH] = (int32) Iacref;
        log2[k % LOG_LENGTH] = (int32) I_error;
    }

    GPIO_setLow(myGpio, GPIO_DBG_ADC_Read);

    // Clear ADCINT1 flag reinitialize for next SOC
    ADC_clearIntFlag(myAdc, ADC_IntNumber_1);
    // Acknowledge interrupt to PIE
    PIE_clearInt(myPie, PIE_GroupNumber_10);

    // From eg
    PWM_clearIntFlag(myPwm1);

    return;
}

void updatePwmDuty(void)
{
    if (pwm_en == 1 && stop_error == 0)
    {

        if (graceful_shutdown == 1)
        {
            //Discharge cap

            if (GPIO_getData(myGpio, GPIO_DC_Relay) == 0)
            { //make sure DC is open before discharge
                pwm_duty = _IQsat(_IQ(1.0) - _IQdiv(Vdc,Vdcref), _IQ(1.0),
                                  _IQ(0.3)); //1 - 0.7*(Vdc/Vdcref)
            }
            else
            {
                pwm_duty = _IQ(0.0);
            }

            pwm_out = IQtoINT(_IQmpy(pwm_duty, _IQ(EPWM_TIMER_TBPRD)));
            PWM_setCmpA(myPwm1, pwm_out); //Q3Q4
            PWM_setCmpA(myPwm2, 0); //Q1Q2

        }
        else
        {
            if (Vacref >= _IQ(0))
            {
                pwm_duty = _IQdiv(Vacref, Vdcref);
            }
            else
            {
                pwm_duty = _IQdiv(-Vacref, Vdcref);
            }

            pwm_duty = _IQsat(pwm_duty, _IQ(0.95), _IQ(0.05));
            pwm_out = IQtoINT(_IQmpy(pwm_duty, _IQ(EPWM_TIMER_TBPRD)));

            if (Vacref >= _IQ(0))
            {
                PWM_setCmpA(myPwm1, pwm_out); //Q3Q4
                PWM_setCmpA(myPwm2, 0); //Q1Q2
            }
            else
            {
                PWM_setCmpA(myPwm1, 0); //Q3Q4
                PWM_setCmpA(myPwm2, pwm_out);

            }
        }

#ifdef SFRA_ENABLE
        SFRA_IQ_COLLECT(&pwm_duty,&Vdc);
#endif

    }
    else
    {
        PWM_setCmpA(myPwm1, 0);
        PWM_setCmpA(myPwm2, 0);
    }

    return;
}

void voltage_controller(void)
{
    if (graceful_shutdown > 0 || stop_error > 0)
    {
        Idcref = _IQ(0.0);
        Iacref = _IQ(0.0);
    }
    else
    {

        // Calculate the error signal
        V_error = Vdc - Vdcref;

        cntl_vctrl_vars1.Ref = _IQtoIQ24(V_error);
        cntl_vctrl_vars1.Fdbk = _IQ24(0.0);
        CNTL_2P2Z_IQ_FUNC(&cntl_vctrl_coeffs1, &cntl_vctrl_vars1);

        Idcref = cntl_vctrl_vars1.Out;

        // Calculate the AC current reference signal
        if (Vacmag < _IQ(0.1)) //Prevent divide by zero
        {
            Iacref = _IQmpy(_IQmpy(_IQ(20.0), spll.cos), _IQmpy(Idcref,Vdc));
        }
        else
        {
            Iacref = _IQmpy(_IQmpy(_IQ(2.0), spll.cos),
                            _IQmpy(Idcref,_IQdiv(Vdc,Vacmag)));
        }

        // Optional Current ref Saturation
        Iacref = _IQsat(Iacref, _IQ(45), _IQ(-45));

        // TEST 643 - Use line below to test Ictrl with AC waveform
        //Iacref = _IQmpy(_IQ23toIQ(spll.osg_u[0]), _IQ(0.05));
    }
}

void current_controller(void)
{

    // Calculate the voltage feed-forward
    Vff = _IQ23toIQ(spll.osg_u[0]); // OR = _IQmpy(Vacmag, _IQcos(theta_hat)); but more sluggish

    // Calculate the error signal
    I_error = Iacref - Iac;
    //I_errorPU = _IQdiv(I_error, Iacref);

    cntl_ictrl_pi.Ref = _IQtoIQ24(Iacref);
    cntl_ictrl_pi.Fbk = _IQtoIQ24(Iac);
    CNTL_PI_IQ_FUNC(&cntl_ictrl_pi);

    cntl_pr1_vars1.Ref = _IQtoIQ24(I_error);
    cntl_pr1_vars1.Fdbk = _IQ24(0.0);
    CNTL_2P2Z_IQ_FUNC(&cntl_pr1_coeffs1, &cntl_pr1_vars1);

    cntl_pr3_vars1.Ref = _IQtoIQ24(I_error);
    cntl_pr3_vars1.Fdbk = _IQ24(0.0);
    CNTL_2P2Z_IQ_FUNC(&cntl_pr3_coeffs1, &cntl_pr3_vars1);

    cntl_pr5_vars1.Ref = _IQtoIQ24(I_error);
    cntl_pr5_vars1.Fdbk = _IQ24(0.0);
    CNTL_2P2Z_IQ_FUNC(&cntl_pr5_coeffs1, &cntl_pr5_vars1);

    Vhr = _IQ24toIQ(cntl_ictrl_pi.Out) + _IQ24toIQ(cntl_pr1_vars1.Out)
    + _IQ24toIQ(cntl_pr3_vars1.Out + cntl_pr5_vars1.Out);
    // Optional Current Ctrl out Saturation
    Vhr = _IQsat(Vhr, _IQ(100), _IQ(-100));

    // Output to PWM
    Vacref = Vff + Vhr;

    // TEST 622  Open loop feed forward of Current Reference
    //Vacref = _IQ23toIQ(spll.osg_u[0]);

    // TEST 623  PLL DQ ref frame test
    //Vacref =  _IQ23toIQ(_IQ23mpy(_IQ23mag(spll.u_Q[0], spll.u_D[0]), spll.cos));

}

/**
 *  System Init Routines
 */
void InitGPIOs(void)
{

    // Initialize GPIO for PWM
    GPIO_setPullUp(myGpio, GPIO_Number_0, GPIO_PullUp_Disable);
    GPIO_setPullUp(myGpio, GPIO_Number_1, GPIO_PullUp_Disable);
    GPIO_setMode(myGpio, GPIO_Number_0, GPIO_0_Mode_EPWM1A);
    GPIO_setMode(myGpio, GPIO_Number_1, GPIO_1_Mode_EPWM1B);

    GPIO_setPullUp(myGpio, GPIO_Number_2, GPIO_PullUp_Disable);
    GPIO_setPullUp(myGpio, GPIO_Number_3, GPIO_PullUp_Disable);
    GPIO_setMode(myGpio, GPIO_Number_2, GPIO_2_Mode_EPWM2A);
    GPIO_setMode(myGpio, GPIO_Number_3, GPIO_3_Mode_EPWM2B);

    GPIO_setDirection(myGpio, GPIO_Number_0, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_1, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_2, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_3, GPIO_Direction_Output);

    // Init ADC read pin
    GPIO_setMode(myGpio, GPIO_DBG_ADC_Read, GPIO_28_Mode_GeneralPurpose);
    GPIO_setQualification(myGpio, GPIO_DBG_ADC_Read, GPIO_Qual_ASync);
    GPIO_setDirection(myGpio, GPIO_DBG_ADC_Read, GPIO_Direction_Output);
    GPIO_setLow(myGpio, GPIO_DBG_ADC_Read);

    //Not used GPIO
    //GPIO_setMode(myGpio, GPIO_Number_16, GPIO_16_Mode_TZ2_NOT);
    //GPIO_setDirection(myGpio, GPIO_Number_16, GPIO_Direction_Output);
    //GPIO_setMode(myGpio, GPIO_Number_17, GPIO_17_Mode_TZ2_NOT);
    //GPIO_setDirection(myGpio, GPIO_Number_17, GPIO_Direction_Output);

    // DC Contactor
    GPIO_setLow(myGpio, GPIO_DC_Relay);
    //GPIO_setMode(myGpio, GPIO_Number_33, GPIO_33_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_DC_Relay, GPIO_17_Mode_GeneralPurpose);
    GPIO_setQualification(myGpio, GPIO_DC_Relay, GPIO_Qual_ASync);
    GPIO_setDirection(myGpio, GPIO_DC_Relay, GPIO_Direction_Output);

    // AC  Contactor
    GPIO_setLow(myGpio, GPIO_AC_Relay);
    GPIO_setMode(myGpio, GPIO_AC_Relay, GPIO_32_Mode_GeneralPurpose);
    GPIO_setQualification(myGpio, GPIO_AC_Relay, GPIO_Qual_ASync);
    GPIO_setDirection(myGpio, GPIO_AC_Relay, GPIO_Direction_Output);

    // Pre-Charge Relay
    GPIO_setLow(myGpio, GPIO_Precharge);
    GPIO_setMode(myGpio, GPIO_Precharge, GPIO_0_Mode_GeneralPurpose);
    GPIO_setQualification(myGpio, GPIO_Precharge, GPIO_Qual_ASync);
    GPIO_setDirection(myGpio, GPIO_Precharge, GPIO_Direction_Output);

}

void InitADCs()
{

    // Done in MAIN - Initialise the ADC
    //ADC_enableBandGap(myAdc);
    //ADC_enableRefBuffers(myAdc);
    //ADC_setVoltRefSrc(myAdc, ADC_VoltageRefSrc_Int);
    //ADC_powerUp(myAdc);
    //ADC_enable(myAdc);

    // Configure ADC
    ADC_setIntPulseGenMode(myAdc, ADC_IntPulseGenMode_Prior); //ADCINT1 trips after AdcResults latch
    ADC_enableInt(myAdc, ADC_IntNumber_1);                     //Enabled ADCINT1
    ADC_setIntMode(myAdc, ADC_IntNumber_1, ADC_IntMode_ClearFlag); //Disable ADCINT1 Continuous mode
    ADC_setIntSrc(myAdc, ADC_IntNumber_1, ADC_IntSrc_EOC5); //setup EOC4 to trigger ADCINT1 to fire

    //Note: Channel ADCINA7  will be double sampled to workaround the ADC 1st sample issue for rev0 silicon errata
#ifndef FILTERED_INPUTS
    //PCB direct inputs (not filtered) - use if filter has been physically removed
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_0, ADC_SocChanNumber_A7);//set SOC0 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_1, ADC_SocChanNumber_A7);//set SOC0 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_2, ADC_SocChanNumber_A3);//set SOC1 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_3, ADC_SocChanNumber_A0);//set SOC2 channel select to ADCINA2
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_4, ADC_SocChanNumber_B1);//set SOC1 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_5, ADC_SocChanNumber_B3);//set SOC2 channel select to ADCINA2
#else
    //Filtered inputs
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_0, ADC_SocChanNumber_A6); //set SOC0 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_1, ADC_SocChanNumber_A6); //set SOC0 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_2, ADC_SocChanNumber_A4); //set SOC1 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_3, ADC_SocChanNumber_A2); //set SOC2 channel select to ADCINA2
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_4, ADC_SocChanNumber_B2); //set SOC1 channel select to ADCINA4
    ADC_setSocChanNumber(myAdc, ADC_SocNumber_5, ADC_SocChanNumber_B4); //set SOC2 channel select to ADCINA2
#endif

    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_0, ADC_SocTrigSrc_EPWM1_ADCSOCA); //set SOC0 start trigger on EPWM1A, due to round-robin SOC0 converts first then...
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_1, ADC_SocTrigSrc_EPWM1_ADCSOCA); //set SOC1 start trigger on EPWM1A, due to round-robin SOC0 converts first then SOC1
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_2, ADC_SocTrigSrc_EPWM1_ADCSOCA); //set SOC2 start trigger on EPWM1A, due to round-robin SOC0 converts first then SOC2
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_3, ADC_SocTrigSrc_EPWM1_ADCSOCA); //set SOC3 start trigger on EPWM1A, due to round-robin SOC0 converts first then SOC3
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_4, ADC_SocTrigSrc_EPWM1_ADCSOCA); //set SOC4 start trigger on EPWM1A, due to round-robin SOC0 converts first then SOC4
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_5, ADC_SocTrigSrc_EPWM1_ADCSOCA); //set SOC5 start trigger on EPWM1A, due to round-robin SOC0 converts first then SOC5

    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_0,
                           ADC_SocSampleWindow_7_cycles); //set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_1,
                           ADC_SocSampleWindow_7_cycles); //set SOC1 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_2,
                           ADC_SocSampleWindow_7_cycles); //set SOC2 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_3,
                           ADC_SocSampleWindow_7_cycles); //set SOC3 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_4,
                           ADC_SocSampleWindow_7_cycles); //set SOC4 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_5,
                           ADC_SocSampleWindow_7_cycles); //set SOC5 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)

}

void InitEPwms()
{
    CLK_disableTbClockSync(myClk);

    CLK_enablePwmClock(myClk, PWM_Number_1);
    CLK_enablePwmClock(myClk, PWM_Number_2);

    // Setup Sync for BIBOLAR
//    PWM_setSyncMode(myPwm1, PWM_SyncMode_CounterEqualZero);
//    PWM_setSyncMode(myPwm2, PWM_SyncMode_EPWMxSYNC);
//
//    PWM_setPhase(myPwm1, 0);
//    PWM_setPhase(myPwm2, (EPWM_TIMER_TBPRD*0.5)); // Produces a 180 deg phase shift EPWM_TIMER_TBPRD/2 *360=180
//    PWM_setPhaseDir(myPwm1, PWM_PhaseDir_CountUp);
//    PWM_setPhaseDir(myPwm2, PWM_PhaseDir_CountDown);

    // Allow each timer to be sync'ed
    PWM_enableCounterLoad(myPwm1);
    PWM_enableCounterLoad(myPwm2);

    // Setup TBCLK1
    PWM_setCounterMode(myPwm1, PWM_CounterMode_Up);         // Count up
    PWM_setPeriod(myPwm1, EPWM_TIMER_TBPRD);               // Set timer period
    PWM_disableCounterLoad(myPwm1);                     // Disable phase loading
    PWM_setPhase(myPwm1, 0x0000);                           // Phase is 0
    PWM_setCount(myPwm1, 0x0000);                           // Clear counter
    PWM_setHighSpeedClkDiv(myPwm1, PWM_HspClkDiv_by_2); // Clock ratio to SYSCLKOUT
    PWM_setClkDiv(myPwm1, PWM_ClkDiv_by_2);

    // Setup TBCLK2
    PWM_setCounterMode(myPwm2, PWM_CounterMode_Up);     // Count up
    PWM_setPeriod(myPwm2, EPWM_TIMER_TBPRD);           // Set timer period
    PWM_disableCounterLoad(myPwm2);                     // Disable phase loading
    PWM_setPhase(myPwm2, 0x0000);                       // Phase is 0
    PWM_setCount(myPwm2, 0x0000);                       // Clear counter
    PWM_setHighSpeedClkDiv(myPwm2, PWM_HspClkDiv_by_2); // Clock ratio to SYSCLKOUT
    PWM_setClkDiv(myPwm2, PWM_ClkDiv_by_2);

    //    PWM_setSyncMode(myPwm2, PWM_SyncMode_EPWMxSYNC);

    // Setup shadow register load on ZERO for PWM 1
    PWM_setShadowMode_CmpA(myPwm1, PWM_ShadowMode_Shadow);
    PWM_setShadowMode_CmpB(myPwm1, PWM_ShadowMode_Shadow);
    PWM_setLoadMode_CmpA(myPwm1, PWM_LoadMode_Zero);
    PWM_setLoadMode_CmpB(myPwm1, PWM_LoadMode_Zero);

    // Setup shadow register load on ZERO for PWM 2
    PWM_setShadowMode_CmpA(myPwm2, PWM_ShadowMode_Shadow);
    PWM_setShadowMode_CmpB(myPwm2, PWM_ShadowMode_Shadow);
    PWM_setLoadMode_CmpA(myPwm2, PWM_LoadMode_Zero);
    PWM_setLoadMode_CmpB(myPwm2, PWM_LoadMode_Zero);

    // Set Compare values
    PWM_setCmpA(myPwm1, 0);    // Set compare A value
    PWM_setCmpB(myPwm1, 0);    // Set Compare B value
    // Set Compare values
    PWM_setCmpA(myPwm2, 0);    // Set compare A value
    PWM_setCmpB(myPwm2, 0);    // Set Compare B value

    // Set actions
    PWM_setActionQual_Zero_PwmB(myPwm1, PWM_ActionQual_Set); // Set PWM1A on Zero
    PWM_setActionQual_CntUp_CmpA_PwmB(myPwm1, PWM_ActionQual_Clear); // Clear PWM1A on event A, up count

    PWM_setActionQual_Zero_PwmA(myPwm1, PWM_ActionQual_Clear); // Set PWM1B on Zero
    PWM_setActionQual_CntUp_CmpA_PwmA(myPwm1, PWM_ActionQual_Set); // Clear PWM1B on event B, up count

    // Set actions
    PWM_setActionQual_Zero_PwmB(myPwm2, PWM_ActionQual_Set); // Set PWM2A on Zero
    PWM_setActionQual_CntUp_CmpA_PwmB(myPwm2, PWM_ActionQual_Clear); // Clear PWM2A on event A, up count

    PWM_setActionQual_Zero_PwmA(myPwm2, PWM_ActionQual_Clear); // Set PWM2B on Zero
    PWM_setActionQual_CntUp_CmpA_PwmA(myPwm2, PWM_ActionQual_Set); // Clear PWM2B on event B, up count

    // Active high complementary PWMs
    //Setup the Q34 deadband
    PWM_setDeadBandOutputMode(
            myPwm1, PWM_DeadBandOutputMode_EPWMxA_Rising_EPWMxB_Falling);
    PWM_setDeadBandPolarity(myPwm1, PWM_DeadBandPolarity_EPWMxB_Inverted);
    PWM_setDeadBandInputMode(myPwm1,
                             PWM_DeadBandInputMode_EPWMxA_Rising_and_Falling);
    PWM_setDeadBandRisingEdgeDelay(myPwm1, EPWM_MIN_DB);
    PWM_setDeadBandFallingEdgeDelay(myPwm1, EPWM_MIN_DB);

    // Setup the Q12 deadband
    PWM_setDeadBandOutputMode(
            myPwm2, PWM_DeadBandOutputMode_EPWMxA_Rising_EPWMxB_Falling);
    PWM_setDeadBandPolarity(myPwm2, PWM_DeadBandPolarity_EPWMxB_Inverted);
    PWM_setDeadBandInputMode(myPwm2,
                             PWM_DeadBandInputMode_EPWMxA_Rising_and_Falling);
    PWM_setDeadBandRisingEdgeDelay(myPwm2, EPWM_MIN_DB);
    PWM_setDeadBandFallingEdgeDelay(myPwm2, EPWM_MIN_DB);

    EPwm1_DB_Direction = DB_UP;
    EPwm2_DB_Direction = DB_UP;

    // Interrupt where we will change the Compare Values

    PWM_enableSocAPulse(myPwm1);
    //USE PWM to start adc isr
    PWM_setSocAPulseSrc(myPwm1, PWM_SocPulseSrc_CounterEqualPeriod); // Select SOC from from CPMA on upcount
    PWM_setSocAPeriod(myPwm1, PWM_SocPeriod_FirstEvent);

    //PWM_setIntMode(myPwm1, PWM_IntMode_CounterEqualZero);   // Select INT on Zero event
    //PWM_enableInt(myPwm1);                                  // Enable INT
    //PWM_setIntPeriod(myPwm1, PWM_IntPeriod_FirstEvent);     // Generate INT on 3rd event

    //PWM_setIntMode(myPwm2, PWM_IntMode_CounterEqualZero);   // Select INT on Zero event
    //PWM_enableInt(myPwm2);                                  // Enable INT
    //PWM_setIntPeriod(myPwm2, PWM_IntPeriod_FirstEvent);     // Generate INT on 3rd event

    CLK_enableTbClockSync(myClk);
}

interrupt void wakeint_isr(void)
{

    //Precharge
    GPIO_setLow(myGpio, GPIO_Precharge);

    //AC Contactor
    GPIO_setLow(myGpio, GPIO_AC_Relay);

    //DC Contactor
    GPIO_setLow(myGpio, GPIO_DC_Relay);

    state = 0;
    stop_error = 12;    // 12 - System hang error
    pwm_en = 0;
    ctrl_en = 0;

    // do not acknowledge this interrupt to get more from group 1
    // if halt is desired
    PIE_clearInt(myPie, PIE_GroupNumber_1);
}

// Interrupt routines
interrupt void epwm1_timer_isr(void)
{

}

interrupt void cpu_timer1_isr(void)
{
    system_uptime++;
}

interrupt void stkov_isr(void)
{
    stop_error = 11; //11- System stack overflow error
    stkov_error = 1;
}

void InitSFRA(void)
{

#ifdef SFRA_ENABLE

    //SFRA Object Initialization
    //Specify the injection amplitude
    SFRA1.amplitude=_IQ26(0.01);
    //Specify the length of SFRA
    SFRA1.Vec_Length=SFRA_FREQ_LENGTH;
    //Specify the SFRA ISR Frequency
    SFRA1.ISR_Freq=SFRA_ISR_FREQ;
    //Specify the Start Frequency of the SFRA analysis
    SFRA1.Freq_Start=SFRA_FREQ_START;
    //Specify the Frequency Step
    SFRA1.Freq_Step=SFREQ_STEP_MULTIPLY;
    //Assign array location to Pointers in the SFRA object
    SFRA1.FreqVect=FreqVect;
    SFRA1.GH_MagVect=OL_MagVect;
    SFRA1.GH_PhaseVect=OL_PhaseVect;
    SFRA1.H_MagVect=Plant_MagVect;
    SFRA1.H_PhaseVect=Plant_PhaseVect;
    SFRA_IQ_INIT(&SFRA1);
#endif

}

void InitControllers(void)
{
    //Set up Software PLL (twice)
    SPLL_1ph_SOGI_IQ_init(GRID_FREQ, SPLL_SOGI_Q((float) (1.0 / ISR_FREQUENCY)),
                          &spll);
    SPLL_1ph_SOGI_IQ_coeff_update(((float) (1.0 / ISR_FREQUENCY)),
                                  (float) (2 * PI * GRID_FREQ), &spll);

    //added to preinit pll lib
    spll.u[0] = _IQ23(0);
    SPLL_1ph_SOGI_IQ_FUNC(&spll);

    //Do again to ensure consistent memory setup
    SPLL_1ph_SOGI_IQ_init(GRID_FREQ, SPLL_SOGI_Q((float) (1.0 / ISR_FREQUENCY)),
                          &spll);
    SPLL_1ph_SOGI_IQ_coeff_update(((float) (1.0 / ISR_FREQUENCY)),
                                  (float) (2 * PI * GRID_FREQ), &spll);
    //spll_lpf_coeff.B0_lf=B0_LPF;
    //spll_lpf_coeff.B1_lf=B1_LPF;
    //spll_lpf_coeff.A1_lf=A1_LPF;

    spll.u[0] = _IQ23(0);
    SPLL_1ph_SOGI_IQ_FUNC(&spll);

#ifdef hinf
    // Use hinf for I controller filter
    CNTL_2P2Z_IQ_COEFFS_init(&cntl_ictrl_coeffs1);
    cntl_ictrl_coeffs1.Coeff_A1 = _IQ24(1);
    cntl_ictrl_coeffs1.Coeff_A2 = _IQ24(-7.618);
    cntl_ictrl_coeffs1.Coeff_A3 = _IQ24(25.66);
    cntl_ictrl_coeffs1.Coeff_A4 = _IQ24(50);
    cntl_ictrl_coeffs1.Coeff_A5 = _IQ24(61.68);
    cntl_ictrl_coeffs1.Coeff_A6 = _IQ24(49.36);
    cntl_ictrl_coeffs1.Coeff_A7 = _IQ24(25.02);
    cntl_ictrl_coeffs1.Coeff_A8 = _IQ24(7.342);

    cntl_ictrl_coeffs1.Coeff_B0 = _IQ24(0.5162);
    cntl_ictrl_coeffs1.Coeff_B1 = _IQ24(0.9537);
    cntl_ictrl_coeffs1.Coeff_B2 = _IQ24(1.457);
    cntl_ictrl_coeffs1.Coeff_B3 = _IQ24(3.987);
    cntl_ictrl_coeffs1.Coeff_B4 = _IQ24(25.62);
    cntl_ictrl_coeffs1.Coeff_B5 = _IQ24(53.45);
    cntl_ictrl_coeffs1.Coeff_B6 = _IQ24(59.4);
    cntl_ictrl_coeffs1.Coeff_B7 = _IQ24(37.77);
    cntl_ictrl_coeffs1.Coeff_B8 = _IQ24(13.03);
    CNTL_2P2Z_IQ_VARS_init(&cntl_ictrl_vars1);
#endif

#ifdef pr

    CNTL_2P2Z_IQ_COEFFS_init(&cntl_vctrl_coeffs1);
    cntl_vctrl_coeffs1.Coeff_A1 = _IQ24(1);
    cntl_vctrl_coeffs1.Coeff_A2 = _IQ24(-1);
    cntl_vctrl_coeffs1.Coeff_B0 = _IQ24(0);
    cntl_vctrl_coeffs1.Coeff_B1 = _IQ24(-7.5118);
    cntl_vctrl_coeffs1.Coeff_B2 = _IQ24(7.4977);
    cntl_vctrl_coeffs1.Max = _IQ24(25);
    cntl_vctrl_coeffs1.Min = _IQ24(-25);

    CNTL_PI_IQ_init(&cntl_ictrl_pi);
    cntl_ictrl_pi.Ki = _IQ24(1.5);
    cntl_ictrl_pi.Kp = _IQ24(0.5415);
    cntl_ictrl_pi.Umax = _IQ24(70);
    cntl_ictrl_pi.Umin = _IQ24(-70);

    CNTL_2P2Z_IQ_COEFFS_init(&cntl_pr1_coeffs1);
    cntl_pr1_coeffs1.Coeff_A1 = _IQ24(1.9997532751092);
    cntl_pr1_coeffs1.Coeff_A2 = _IQ24(-1);
    cntl_pr1_coeffs1.Coeff_B0 = _IQ24(0.541553621692344);
    cntl_pr1_coeffs1.Coeff_B1 = _IQ24(-1.08286639847163);
    cntl_pr1_coeffs1.Coeff_B2 = _IQ24(0.541446378307656);
    cntl_pr1_coeffs1.Max = _IQ24(70);
    cntl_pr1_coeffs1.Min = _IQ24(-70);

    CNTL_2P2Z_IQ_VARS_init(&cntl_pr1_vars1);

    CNTL_2P2Z_IQ_COEFFS_init(&cntl_pr3_coeffs1);
    cntl_pr3_coeffs1.Coeff_A1 = _IQ24(1.99778057115952);
    cntl_pr3_coeffs1.Coeff_A2 = _IQ24(-1);
    cntl_pr3_coeffs1.Coeff_B0 = _IQ24(0.104997225713949);
    cntl_pr3_coeffs1.Coeff_B1 = _IQ24(-0.199778057115952);
    cntl_pr3_coeffs1.Coeff_B2 = _IQ24(0.0950027742860506);
    cntl_pr3_coeffs1.Max = _IQ24(35);
    cntl_pr3_coeffs1.Min = _IQ24(-35);

    CNTL_2P2Z_IQ_VARS_init(&cntl_pr3_vars1);

    CNTL_2P2Z_IQ_COEFFS_init(&cntl_pr5_coeffs1);
    cntl_pr5_coeffs1.Coeff_A1 = _IQ24(1.99384099520882);
    cntl_pr5_coeffs1.Coeff_A2 = _IQ24(-1);
    cntl_pr5_coeffs1.Coeff_B0 = _IQ24(0.074992301244011);
    cntl_pr5_coeffs1.Coeff_B1 = _IQ24(-0.139568869664617);
    cntl_pr5_coeffs1.Coeff_B2 = _IQ24(0.065007698755989);
    cntl_pr5_coeffs1.Max = _IQ24(17);
    cntl_pr5_coeffs1.Min = _IQ24(-17);

    CNTL_2P2Z_IQ_VARS_init(&cntl_pr5_vars1);

    CNTL_2P2Z_IQ_COEFFS_init(&cntl_pr7_coeffs1);
    cntl_pr7_coeffs1.Coeff_A1 = _IQ24(1.98794616811528);
    cntl_pr7_coeffs1.Coeff_A2 = _IQ24(-1);
    cntl_pr7_coeffs1.Coeff_B0 = _IQ24(0.0749849327101441);
    cntl_pr7_coeffs1.Coeff_B1 = _IQ24(-0.13915623176807);
    cntl_pr7_coeffs1.Coeff_B2 = _IQ24(0.0650150672898559);
    cntl_pr7_coeffs1.Max = _IQ24(10);
    cntl_pr7_coeffs1.Min = _IQ24(-10);

    CNTL_2P2Z_IQ_VARS_init(&cntl_pr7_vars1);

#endif
}

//===========================================================================
// End of program.
//===========================================================================
