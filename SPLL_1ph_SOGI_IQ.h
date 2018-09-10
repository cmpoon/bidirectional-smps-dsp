#ifndef SPLL_1ph_SOGI_IQ_H_
#define SPLL_1ph_SOGI_IQ_H_

#define SPLL_SOGI_Q _IQ23
#define SPLL_SOGI_Qmpy _IQ23mpy
#define SPLL_SOGI_SINE _IQ23sin
#define SPLL_SOGI_COS  _IQ23cos

//*********** Structure Definition ********//
typedef struct{
	long	osg_k;
	long	osg_x;
	long	osg_y;
	long	osg_b0;
	long	osg_b2;
	long	osg_a1;
	long	osg_a2;
	long	osg_qb0;
	long	osg_qb1;
	long	osg_qb2;
}SPLL_1ph_SOGI_IQ_OSG_COEFF;

typedef struct{
	long	B1_lf;
	long	B0_lf;
	long	A1_lf;
}SPLL_1ph_SOGI_IQ_LPF_COEFF;

typedef struct{
	long	u[3];  // Ac Input
	long   osg_u[3];
	long   osg_qu[3];
	long   u_Q[2];
	long   u_D[2];
	long   ylf[2];
	long   fo; // output frequency of PLL
	long   fn; //nominal frequency
	long	theta[2];
	long	cos;
	long	sin;
	long   delta_T;
	SPLL_1ph_SOGI_IQ_OSG_COEFF osg_coeff;
	SPLL_1ph_SOGI_IQ_LPF_COEFF lpf_coeff;
}SPLL_1ph_SOGI_IQ;

//*********** Function Declarations *******//
void SPLL_1ph_SOGI_IQ_init(int Grid_freq, long DELTA_T, SPLL_1ph_SOGI_IQ *spll);
void SPLL_1ph_SOGI_IQ_coeff_update(float delta_T, float wn, SPLL_1ph_SOGI_IQ *spll);
void SPLL_1ph_SOGI_IQ_FUNC(SPLL_1ph_SOGI_IQ *spll1);

//*********** Macro Definition ***********//
#define SPLL_1ph_SOGI_IQ_MACRO(v) 																																																				\
	v.osg_u[0]=SPLL_SOGI_Qmpy(v.osg_coeff.osg_b0,(v.u[0]-v.u[2]))+SPLL_SOGI_Qmpy(v.osg_coeff.osg_a1,v.osg_u[1])+SPLL_SOGI_Qmpy(v.osg_coeff.osg_a2,v.osg_u[2]); 																					\
	v.osg_u[2]=v.osg_u[1];																																																						\
	v.osg_u[1]=v.osg_u[0];																																																						\
	v.osg_qu[0]=SPLL_SOGI_Qmpy(v.osg_coeff.osg_qb0,v.u[0])+SPLL_SOGI_Qmpy(v.osg_coeff.osg_qb1,v.u[1])+SPLL_SOGI_Qmpy(v.osg_coeff.osg_qb2,v.u[2])+SPLL_SOGI_Qmpy(v.osg_coeff.osg_a1,v.osg_qu[1])+SPLL_SOGI_Qmpy(v.osg_coeff.osg_a2,v.osg_qu[2]); \
	v.osg_qu[2]=v.osg_qu[1];																																																					\
	v.osg_qu[1]=v.osg_qu[0];																																																					\
	v.u[2]=v.u[1];																																																								\
	v.u[1]=v.u[0];																																																								\
	v.u_Q[0]=SPLL_SOGI_Qmpy(v.cos,v.osg_u[0])+SPLL_SOGI_Qmpy(v.sin,v.osg_qu[0]);																																								\
	v.u_D[0]=SPLL_SOGI_Qmpy(v.cos,v.osg_qu[0])-SPLL_SOGI_Qmpy(v.sin,v.osg_u[0]);																																								\
	v.ylf[0]=v.ylf[1]+SPLL_SOGI_Qmpy(v.lpf_coeff.B0_lf,v.u_Q[0])+SPLL_SOGI_Qmpy(v.lpf_coeff.B1_lf,v.u_Q[1]);																																	\
	v.ylf[1]=v.ylf[0];																																																							\
	v.u_Q[1]=v.u_Q[0];																																																							\
	v.fo=v.fn+v.ylf[0]; 																																																						\
	v.theta[0]=v.theta[1]+SPLL_SOGI_Qmpy(SPLL_SOGI_Qmpy(v.fo,v.delta_T),SPLL_SOGI_Qmpy(SPLL_SOGI_Q(2.0),SPLL_SOGI_Q(3.1415926))); 																																				\
	if(v.theta[0]>SPLL_SOGI_Qmpy(SPLL_SOGI_Q(2.0),SPLL_SOGI_Q(3.1415926))) 																																																	\
		v.theta[0]=SPLL_SOGI_Q(0.0); 																																																			\
	v.theta[1]=v.theta[0]; 																																																						\
	v.sin=SPLL_SOGI_SINE(v.theta[0]); 																																																			\
	v.cos=SPLL_SOGI_COS(v.theta[0]);


#endif /* SPLL_1ph_SOGI_IQ_H_ */
