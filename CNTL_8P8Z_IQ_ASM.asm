
    .def _CNTL_8P8Z_IQ_ASM

_CNTL_8P8Z_IQ_ASM

	; because of C calling convention
	; XAR4 has the cntl8p8z coeff structure pointer
	; XAR5 has the cntl8p8z data buff internal pointer
	; ACC has the reference
	; *-SP[4] has the feedback value

	; calculate error (Ref - Fdbk)
	MOV     AR0,#30
    MOVL    ACC,*+XAR5[AR0]
    MOV     AR0,#32
	SUBL	ACC, *+XAR5[AR0]		; ACC = Ref(Q24) - Fdbk(Q24)= error(Q24)
	LSL     ACC, #6					; Logical left shift by 6, Q{24}<<6 -> Q{30}

; store error in DBUFF
	MOVL	*+XAR5[6], ACC			; e(n) = ACC = error Q{30}

	ZAPA
; compute 3P3Z filter
	MOV		AR0,#28
	MOVL	XT, *+XAR5[AR0]			; XT  = e(n-8)
	QMPYL	P, XT, *XAR4++			; P   = e(n-8)Q30*B3{Q24} = I8Q22

	MOV		AR0,#26					;
	MOVDL	XT, *+XAR5[AR0]			; XT  = e(n-7), e(n-8) = e(n-7)
	QMPYAL	P, XT, *XAR4++ 			; P   = e(n-7)Q30*B2{Q24} = Q24, ACC=e(n-7)*B8

	MOV		AR0,#24					;
	MOVDL	XT, *+XAR5[AR0]			; XT  = e(n-6), e(n-7) = e(n-6)
	QMPYAL	P, XT, *XAR4++ 			; P   = e(n-6)Q30*B7{Q24} = Q24, ACC=e(n-7)*B8 + e(n-7)*B7

	MOV		AR0,#22					;
	MOVDL	XT, *+XAR5[AR0]			; XT  = e(n-5), e(n-6) = e(n-5)
	QMPYAL	P, XT, *XAR4++ 			; P   = e(n-5)Q30*B6{Q24} = Q24, ACC=e(n-7)*B8 + ... + e(n-6)*B6

	MOV		AR0,#20					;
	MOVDL	XT, *+XAR5[AR0]			; XT  = e(n-4), e(n-5) = e(n-4)
	QMPYAL	P, XT, *XAR4++ 			; P   = e(n-4)Q30*B4{Q24} = Q24, ACC=e(n-7)*B8 + ... + e(n-5)*B5

	MOV		AR0,#18					;
	MOVDL	XT, *+XAR5[AR0]			; XT  = e(n-3), e(n-4) = e(n-3)
	QMPYAL	P, XT, *XAR4++ 			; P   = e(n-3)Q30*B3{Q24} = Q24, ACC=e(n-7)*B8 + ... + e(n-4)*B4

	MOV		AR0,#16					;
	MOVDL	XT, *+XAR5[AR0]			; XT  = e(n-2), e(n-3) = e(n-2)
	QMPYAL	P, XT, *XAR4++ 			; P   = e(n-2)Q30*B2{Q24} = Q24, ACC=e(n-7)*B8 + ... + e(n-3)*B3

	MOV		AR0,#14
	MOVDL	XT, *+XAR5[AR0]	 		; XT  = e(n-1), e(n-2) = e(n-1)
	QMPYAL	P, XT, *XAR4++ 			; P   = e(n-1)Q30*B1{Q24}= Q24, ACC = e(n-7)*B8 + ... + e(n-3)*B3 + e(n-2)*B2

	MOV		AR0,#12
	MOVDL	XT, *+XAR5[AR0]			; XT  = e(n),   e(n-1) = e(n)
	QMPYAL	P, XT, *XAR4++			; P   = e(n)Q30*B0{Q24}  = Q24, ACC = e(n-8)*B8 + e(n-7)*B7 + e(n-6)*B6 + e(n-5)*B5 + e(n-4)*B4 + e(n-3)*B3 + e(n-2)*B2 + e(n-1)*B1

	MOV		AR0,#10
	MOVDL	XT, *+XAR5[AR0]			; XT  = u(n-3), u(n-4) = u(n-3)
	QMPYAL	P, XT, *XAR4++			; P   = u(n-3)*A2, ACC = e(n-7)*B8 + ... + e(n-1)*B1 + u(n-4)*A4

	MOV		AR0,#8
	MOVDL	XT, *+XAR5[AR0]			; XT  = u(n-3), u(n-4) = u(n-3)
	QMPYAL	P, XT, *XAR4++			; P   = u(n-3)*A2, ACC = e(n-7)*B8 + ... + e(n-1)*B1 + u(n-4)*A4

	MOVDL	XT,*+XAR5[6]			; XT  = u(n-4), u(n-4) = u(n-3)
	QMPYAL	P, XT, *XAR4++			; P   = u(n-4)*A2, ACC = e(n-7)*B8 + ... + e(n-1)*B1 + u(n-4)*A4

	MOVDL	XT,*+XAR5[4]			; XT  = u(n-3), u(n-4) = u(n-3)
	QMPYAL	P, XT, *XAR4++			; P   = u(n-3)*A2, ACC = e(n-7)*B8 + ... + e(n-1)*B1 + u(n-4)*A4

	MOVDL	XT,*+XAR5[2]			; XT  = u(n-2), u(n-3) = u(n-2)
	QMPYAL	P, XT, *XAR4++			; P   = u(n-2)*A2, ACC = e(n-3)B3 +e(n-2)*B2 + e(n-1)*B1 + e(n)*B0 + u(n-3)*A3

	MOVDL	XT,*+XAR5[0]			; XT  = u(n-1), u(n-2) = u(n-1)
	QMPYAL	P, XT, *XAR4++			; P   = u(n-1)*A1, ACC = e(n-3)B3 +e(n-2)*B2 + e(n-1)*B1 + e(n)*B0 + u(n-3)*A3 + u(n-2)*A2

	ADDL	ACC, @P					; ACC =  e(n-3)B3 +e(n-2)*B2 + e(n-1)*B1 + e(n)*B0 + u(n-3)*A3 + u(n-2)*A2 + u(n-1)*A1
    LSL     ACC, #2                 ; IQ22 to IQ24
; scale u(n):Q24, saturate (max>u(n)>min0), and save history

	MINL	ACC, *XAR4++ 			; saturate to < max (Q24)
	MAXL	ACC, *XAR4++            ; saturate to the internal min >internal min (-1.0, Q24)
	MOVL    @XAR6, ACC				; save this value temporaily in XAR6

; Convert the internal u(n) to Q30 format and store in the data buffer
	LSL     ACC, #6				    ; Logical left shift by 6, Q{24}<<6 -> Q{30}
	MOVL	*XAR5, ACC				; u(n-1) = u(n) = ACC

	MOVL    ACC,@XAR6				; load the temporaily saved value in Q24 from XAR1 to ACC
	MAXL	ACC, *XAR4 				; saturate to > output min (Q24)

; write controller result to output terminal (Q24)
    MOV     AR0,#34
    MOVL    *+XAR5[AR0],ACC
	; ACCC has the output value


	LRETR
