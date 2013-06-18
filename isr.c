// isr.c
//
//
#include "syslog.h"
//
//
//
void NmiSR(void){
  syslog(LOG_PRINT,"NmiSR:");
  while(1){ }
}
//
//
void FaultISR(void){
  int* BFAR;
  BFAR = (int*)0xE000ED38;

  syslog(LOG_PRINT,"FaultISR: xxx=0x%X",*(int*)(0xE000ED28));
  syslog(LOG_PRINT,"FaultISR: BFAR=0x%X",*BFAR);
  while(1){ }
}
//
//
void IntDefaultHandler(void){
  syslog(LOG_PRINT,"IntDefaultHandler:");
  while(1){ }
}
void mpu_isr(void){}
void usage_isr(void){}
void bus_isr(void){}
//
//
// These 4 functions are defined in liblm3s6965.o
//
#if 0
void systick_isr(void){ while(1); }
void uart0_isr(void){ while(1); }
void uart1_isr(void){ while(1); }
void uart2_isr(void){ while(1); }
#endif
