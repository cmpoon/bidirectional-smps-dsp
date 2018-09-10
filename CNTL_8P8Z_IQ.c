//#include "Solar_IQ.h"
#include "IQmathLib.h"
#include "CNTL_8P8Z_IQ.h"

//*********** Structure Init Function ****//
void CNTL_8P8Z_IQ_VARS_init(CNTL_8P8Z_IQ_VARS *k){
	// Initialize variables
	k->Ref = 0;
	k->Fdbk = 0;
	k->Errn = 0;
	k->Errn1 = 0;
	k->Errn2 = 0;
    k->Errn3 = 0;
    k->Errn4 = 0;
    k->Errn5 = 0;
    k->Errn6 = 0;
    k->Errn7 = 0;
    k->Errn8 = 0;
	k->Out = 0;
	k->Out1 = 0;
	k->Out2 = 0;
    k->Out3 = 0;
    k->Out4 = 0;
    k->Out5 = 0;
    k->Out6 = 0;
    k->Out7 = 0;
    k->Out8 = 0;
}

void CNTL_8P8Z_IQ_COEFFS_init(CNTL_8P8Z_IQ_COEFFS *v){
	// Initialize coefficients
    v->Coeff_B8 = 0;
    v->Coeff_B7 = 0;
    v->Coeff_B6 = 0;
    v->Coeff_B5 = 0;
    v->Coeff_B4 = 0;
    v->Coeff_B3 = 0;
	v->Coeff_B2 = 0;
	v->Coeff_B1 = 0;
	v->Coeff_B0 = 0;
    v->Coeff_A8 = 0;
    v->Coeff_A7 = 0;
    v->Coeff_A6 = 0;
    v->Coeff_A5 = 0;
    v->Coeff_A4 = 0;
    v->Coeff_A3 = 0;
	v->Coeff_A2 = 0;
	v->Coeff_A1 = 0;
	// IMin cannot be lower than -0.9
	v->IMin = _IQ24(-0.9);
	v->Max = _IQ24(1.0);
	v->Min = (0);
}

//*********** Function Definition ********//

// Calculates a third order control law with IIR filter and programmable output saturation.
// @param CNTL_8P8Z_IQ_COEFFS structure with proper coefficient values.
// @param CNTL_8P8Z_IQ_VARS structure with internal & output values.
// @return CNTL_8P8Z_IQ_VARS Out parameter.

void CNTL_8P8Z_IQ_FUNC(CNTL_8P8Z_IQ_COEFFS *v, CNTL_8P8Z_IQ_VARS *k){
	/* Calculate error */
	k->Errn = k->Ref - k->Fdbk;
	k->Out = _IQ24mpy(v->Coeff_A8,k->Out8)+_IQ24mpy(v->Coeff_A7,k->Out7)+_IQ24mpy(v->Coeff_A6,k->Out6)+_IQ24mpy(v->Coeff_A5,k->Out5)
	+ _IQ24mpy(v->Coeff_A4,k->Out4)+_IQ24mpy(v->Coeff_A3,k->Out3)+_IQ24mpy(v->Coeff_A2,k->Out2) + _IQ24mpy(v->Coeff_A1 , k->Out1)
	+ _IQ24mpy(v->Coeff_B8 ,k->Errn8)+_IQ24mpy(v->Coeff_B7 ,k->Errn7)+_IQ24mpy(v->Coeff_B6 ,k->Errn6)+_IQ24mpy(v->Coeff_B5 ,k->Errn5)
	+ _IQ24mpy(v->Coeff_B4 ,k->Errn4)+_IQ24mpy(v->Coeff_B3 ,k->Errn3)+_IQ24mpy(v->Coeff_B2 ,k->Errn2) + _IQ24mpy(v->Coeff_B1 , k->Errn1)
	+ _IQ24mpy(v->Coeff_B0 , k->Errn);

	/* Update error values */
    k->Errn8 = k->Errn7;
    k->Errn7 = k->Errn6;
    k->Errn6 = k->Errn5;
    k->Errn5 = k->Errn4;
    k->Errn4 = k->Errn3;
    k->Errn3 = k->Errn2;
	k->Errn2 = k->Errn1;
	k->Errn1 = k->Errn;

	/* Determine new output */
	k->Out = (k->Out < v->Max) ? k->Out : v->Max;
	k->Out = (k->Out > v->IMin) ? k->Out : v->IMin;

	/* Store outputs */
    k->Out8 = k->Out7;
    k->Out7 = k->Out6;
    k->Out6 = k->Out5;
    k->Out5 = k->Out4;
    k->Out4 = k->Out3;
    k->Out3 = k->Out2;
	k->Out2 = k->Out1;
	k->Out1 = k->Out;
	/* Saturated output */
	k->Out = ((k->Out > v->Min) ? k->Out : v->Min);
}

