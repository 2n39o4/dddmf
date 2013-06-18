#include "inc/lm3s6965.h" 
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void uC_init_CLOCK(int freq){
  int i;
  switch(freq){
  case  8: SysCtlClockSet( SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );
           break;
  case 50: SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );
           break;
  default: SysCtlClockSet( SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );
  }
  for(i=0;i<0x00010000;i++);
}

unsigned int uC_CLOCK(void){ return SysCtlClockGet(); }

