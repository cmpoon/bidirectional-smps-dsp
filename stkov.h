/*
 * stkov.h
 *
 *  Created on: 10 Jun 2018
 *      Author: Chung
 */

#ifndef STKOV_
#define STKOV_
//#include <tsk.h>
// C++ Support
#ifdef __cplusplus
extern "C"
{
#endif
// Global Function Prototypes
extern unsigned int STKOV_initSystemStack(unsigned long, unsigned long,
                                          unsigned int);
//extern void STKOV_createTaskStack(TSK_Handle);
//extern unsigned int STKOV_initTaskStack(void);
//extern void STKOV_switchTaskStack(TSK_Handle, TSK_Handle);
// Global symbols defined in the linker command file
extern unsigned int HWI_STKBOTTOM;
extern unsigned int HWI_STKTOP;
#ifdef __cplusplus
}
#endif
#endif // end of STKOV_ #ifndef
// end of file stkov.h
