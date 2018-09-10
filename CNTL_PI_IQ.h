#ifndef CNTL_PI_IQ_H_
#define CNTL_PI_IQ_H_

//*********** Structure Definition ********//

// PI control law with programmable output saturation.
// This macro uses CNTL_PI_IQ structures to store coefficients & internal values.
// The structures should be initialized with the supplied CNTL_PI_IQ_INIT macro.
// Within the structure the Max & Min parameters are the output bounds

typedef struct {
	long Ref;   			// Input: reference set-point
	long Fbk;   			// Input: feedback
	long Out;   			// Output: controller output
	long Kp;				// Parameter: proportional loop gain
	long Ki;			    // Parameter: integral gain
	long Umax;				// Parameter: upper saturation limit
	long Umin;				// Parameter: lower saturation limit
	long up;				// Data: proportional term
	long ui;				// Data: integral term
	long v1;				// Data: pre-saturated controller output
	long i1;				// Data: integrator storage: ui(k-1)
	long w1;				// Data: saturation record: [u(k-1) - v(k-1)]
} CNTL_PI_IQ;

//*********** Function Declarations *******//
void CNTL_PI_IQ_init(CNTL_PI_IQ *k);
void CNTL_PI_IQ_FUNC(CNTL_PI_IQ *v);

//*********** Macro Definition ***********//
#define CNTL_PI_IQ_MACRO(v)										\
	/* proportional term */ 									\
	v.up = _IQmpy((v.Ref - v.Fbk),v.Kp);						\
																\
	/* integral term */ 										\
	v.ui = (v.Out == v.v1)?(_IQmpy(v.Ki, v.up)+ v.i1) : v.i1;	\
	v.i1 = v.ui;												\
																\
	/* control output */ 										\
	v.v1 = (v.up + v.ui);										\
	v.Out= _IQsat(v.v1, v.Umax, v.Umin);						\
	/*v.w1 = (v.Out == v.v1) ? _IQ(1.0) : _IQ(0.0);*/			\
	

#endif /* CNTL_PI_H_ */
