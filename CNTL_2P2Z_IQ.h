#ifndef CNTL_2P2Z_IQ_H_
#define CNTL_2P2Z_IQ_H_

//*********** Structure Definition ********//

// Second order control law using an IIR filter structure with programmable output saturation.
// This macro uses CNTL_2P2Z_IQ structures to store coefficients & internal values.
// The structures should be initialized with the supplied CNTL_2P2Z_IQ_INIT macro.
// Within the structure the Max & Min parameters are the output bounds where as the IMin parameter
// is used for saturating the lower bound while keeping an internal history.  The IMin parameter
// should not be lower than -0.9.

typedef struct {
	// Coefficients
	long Coeff_B2;
	long Coeff_B1;
	long Coeff_B0;
	long Coeff_A2;
	long Coeff_A1;

	// Output saturation limits
	long Max;
	long IMin;
	long Min;
} CNTL_2P2Z_IQ_COEFFS;

typedef struct {
	long Out1;
	long Out2;
	// Internal values
	long Errn;
	long Errn1;
	long Errn2;
	// Inputs
	long Ref;
	long Fdbk;
	// Output values
	long Out;
} CNTL_2P2Z_IQ_VARS;

//*********** Function Declarations *******//
void CNTL_2P2Z_IQ_COEFFS_init(CNTL_2P2Z_IQ_COEFFS *v);
void CNTL_2P2Z_IQ_VARS_init(CNTL_2P2Z_IQ_VARS *k);
void CNTL_2P2Z_IQ_FUNC(CNTL_2P2Z_IQ_COEFFS *v, CNTL_2P2Z_IQ_VARS *k);
void CNTL_2P2Z_IQ_ASM(CNTL_2P2Z_IQ_COEFFS *v, CNTL_2P2Z_IQ_VARS *k);

//*********** Macro Definition ***********//
#define CNTL_2P2Z_IQ_MACRO(v, k)																				\
	/* Calculate error */																						\
	k.Errn = k.Ref - k.Fdbk;																					\
	k.Out = _IQ24mpy(v.Coeff_A2,k.Out2) + _IQ24mpy(v.Coeff_A1 , k.Out1) + _IQ24mpy(v.Coeff_B2 ,k.Errn2)	\
		+ _IQ24mpy(v.Coeff_B1 , k.Errn1) + _IQ24mpy(v.Coeff_B0 , k.Errn);										\
																												\
	/* Update error values */																					\
	k.Errn2 = k.Errn1;																							\
	k.Errn1 = k.Errn;																							\
																												\
	/* Determine new output */																					\
	k.Out = (k.Out < v.Max) ? k.Out : v.Max;																	\
	k.Out = (k.Out > v.IMin) ? k.Out : v.IMin;																	\
																												\
	/* Store outputs */																							\
	k.Out2 = k.Out1;																							\
	k.Out1 = k.Out;																								\
	/* Saturated output */																						\
	k.Out = ((k.Out > v.Min) ? k.Out : v.Min);


#endif /* CNTL_2P2Z_IQ_H_ */
