// lm3s6965-systick.c
//
//
#include "inc/lm3s6965.h"
#include "inc/hw_memmap.h" 
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
//
//
extern void toggle_port_f_bit_0(void);
//
//
//
#define UPDATE_TASKS()
#define HEART_BEAT_RATE
//
//
//
//
//
volatile unsigned int systemTimeTicks = 0;
volatile unsigned int systemSeconds = 0;
//
//
//
unsigned int uC_get_system_seconds(void){ return systemSeconds; }
//
//
//
void systick_isr(void){
  systemTimeTicks++;
  if(!(systemTimeTicks < 1000)){
    systemSeconds++;
    systemTimeTicks = 0;
    toggle_port_f_bit_0();
  }
  UPDATE_TASKS();
  //  update_tasks();
}

// initialize system ticker, freq in hertz 
// call from boot_uC()
// example: uC_init_SYSTICK(1000); <- 1000 Hz (1 mS)
//
//
//
void uC_init_SYSTICK(int freq){
  SysTickPeriodSet(SysCtlClockGet() / freq);   // set systick period
  SysTickEnable();
  SysTickIntEnable();
}
