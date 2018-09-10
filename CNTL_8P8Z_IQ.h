#ifndef CNTL_8P8Z_IQ_H_
#define CNTL_8P8Z_IQ_H_

//*********** Structure Definition ********//

// Third order control law using an IIR filter structure with programmable output saturation.
// This macro uses CNTL_8P8Z_IQ structures to store coefficients & internal values.
// The structures should be initialized with the supplied CNTL_8P8Z_IQ_init function.
// Within the structure the Max & Min parameters are the output bounds where as the IMin parameter
// is used for saturating the lower bound while keeping an internal history.  The IMin parameter
// should not be lower than -0.9.

typedef struct {
	// Coefficients
    long Coeff_B8;
    long Coeff_B7;
    long Coeff_B6;
    long Coeff_B5;
    long Coeff_B4;
	long Coeff_B3;
	long Coeff_B2;
	long Coeff_B1;
	long Coeff_B0;

	long Coeff_A8;
	long Coeff_A7;
	long Coeff_A6;
	long Coeff_A5;
	long Coeff_A4;
	long Coeff_A3;
	long Coeff_A2;
	long Coeff_A1;

	// Output saturation limits
	long Max;
	long IMin;
	long Min;
} CNTL_8P8Z_IQ_COEFFS;

typedef struct {
	long Out1;
	long Out2;
	long Out3;
	long Out4;
	long Out5;
	long Out6;
	long Out7;
	long Out8;
	// Internal values
	long Errn;
	long Errn1;
	long Errn2;
	long Errn3;
	long Errn4;
	long Errn5;
	long Errn6;
	long Errn7;
	long Errn8;

	// Inputs
	long Ref;
	long Fdbk;
	// Output values
	long Out;
} CNTL_8P8Z_IQ_VARS;

//*********** Function Declarations *******//
void CNTL_8P8Z_IQ_VARS_init(CNTL_8P8Z_IQ_VARS *k);
void CNTL_8P8Z_IQ_COEFFS_init(CNTL_8P8Z_IQ_COEFFS *v);
void CNTL_8P8Z_IQ_FUNC(CNTL_8P8Z_IQ_COEFFS *v, CNTL_8P8Z_IQ_VARS *k);
void CNTL_8P8Z_IQ_ASM(CNTL_8P8Z_IQ_COEFFS *v, CNTL_8P8Z_IQ_VARS *k);

//*********** Macro Definition ***********//
#define CNTL_8P8Z_IQ_MACRO(v, k)																											\
	/* Calculate error */																											\
	k.Errn = k.Ref - k.Fdbk;																										\
    k->Out = _IQ24mpy(v->Coeff_A8,k->Out8)+_IQ24mpy(v->Coeff_A7,k->Out7)+_IQ24mpy(v->Coeff_A6,k->Out6)+_IQ24mpy(v->Coeff_A5,k->Out5)    \
    + _IQ24mpy(v->Coeff_A4,k->Out4)+_IQ24mpy(v->Coeff_A3,k->Out3)+_IQ24mpy(v->Coeff_A2,k->Out2) + _IQ24mpy(v->Coeff_A1 , k->Out1)   \
    + _IQ24mpy(v->Coeff_B8 ,k->Errn8)+_IQ24mpy(v->Coeff_B7 ,k->Errn7)+_IQ24mpy(v->Coeff_B6 ,k->Errn6)+_IQ24mpy(v->Coeff_B5 ,k->Errn5)   \
    + _IQ24mpy(v->Coeff_B4 ,k->Errn4)+_IQ24mpy(v->Coeff_B3 ,k->Errn3)+_IQ24mpy(v->Coeff_B2 ,k->Errn2) + _IQ24mpy(v->Coeff_B1 , k->Errn1)    \
    + _IQ24mpy(v->Coeff_B0 , k->Errn);																												\
	/* Update error values */																										\
        k->Errn8 = k->Errn7;                                                                                                                \
        k->Errn7 = k->Errn6;                                                                                                                \
        k->Errn6 = k->Errn5;                                                                                                                \
        k->Errn5 = k->Errn4;                                                                                                                \
        k->Errn4 = k->Errn3;                                                                                                                \
        k->Errn3 = k->Errn2;                                                                                                                \
        k->Errn2 = k->Errn1;                                                                                                                \
        k->Errn1 = k->Errn;                                                                                                             \
																																	\
	/* Determine new output */																										\
	k.Out = (k.Out < v.Max) ? k.Out : v.Max;																						\
	k.Out = (k.Out > v.IMin) ? k.Out : v.IMin;																						\
																																	\
	/* Store outputs */																												\
    k->Out8 = k->Out7;                                                                                                              \
    k->Out7 = k->Out6;                                                                                                              \
    k->Out6 = k->Out5;                                                                                                              \
    k->Out5 = k->Out4;                                                                                                              \
    k->Out4 = k->Out3;                                                                                                              \
    k->Out3 = k->Out2;                                                                                                              \
    k->Out2 = k->Out1;                                                                                                              \
    k->Out1 = k->Out;                                                                                                               \
	/* Saturated output */																											\
	k.Out = ((k.Out > v.Min) ? k.Out : v.Min);


#endif /* CNTL_8P8Z_IQ_H_ */
