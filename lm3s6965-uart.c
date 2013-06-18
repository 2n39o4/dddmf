// lm3s6965-uart.c
//
// all uart specific code goes here
//
//
// TODO: What/Why/Where is the SSI used for
// TODO: Any embedded system should run a watch-dog timer
//
// NOTES: What should I name this file.  I like the lib- part BUT the lm3s6965 is not quite
//        correct.  For a long time I thought the com1268 had an lm3s6965 when in fact it
//        is an lm3s6911.  
//
//
///////////////////////////////////////////////////////
//     6911 -> Cortex-M3 based Microcontroller:	     //
//     - 50MHz, 256KB Flash ROM, 64KB SRAM, MPU	     //
//     - 4 Timers				     //
//     - 6 Capture Compare Pins			     //
//     - Ethernet MAC + PHY			     //
//     - 3 * UART				     //
//     - 2 * I2C				     //
//     - 2 * SSI				     //
//     - 2 * Analog Comparator			     //
//     - 46 GPIO Pins				     //
//     - Battery-backed Hibernation Module	     //
//     - Watchdog Timer, Power saving modes,	     //
//       JTAG and Serial Wire debug		     //
//     						     //
//     						     //
//     6965 => Cortex-M3 based Microcontroller:	     //
//     - 50MHz, 256KB Flash ROM, 64KB SRAM, MPU	     //
//     - 4 Timers				     //
//     - 6 Motion Control PWM			     //
//     - 4 Capture Compare Pins			     //
//     - Ethernet MAC + PHY			     //
//     - 3 * UART				     //
//     - 2 * I2C				     //
//     - 1 * SSI				     //
//     - 2 * QEI				     //
//     - 4 * 10-bit ADC				     //
//     - Temperature Sensor			     //
//     - 2 * Analog Comparator			     //
//     - 42 GPIO Pins				     //
//     - Battery-backed Hibernation Module	     //
//     - Watchdog Timer, Power saving modes,	     //
//       JTAG and Serial Wire debug		     //
//     						     //
///////////////////////////////////////////////////////
//
//    KEEP FREE OF LIBC DEPENDENCY !!!!
//#include <stdio.h>
//#include <stdarg.h> 
//#include <string.h>
//#include <stdlib.h>
//
//
//
// Notes: When using a makefile we need to know where the Stellaris library is located
//
//            STELLARISWARE=/home/neon/etc/TI/Luminary/StellarisWare/9453
//        
//        With STELLARISWARE defined additional path info is needed. So I use
//        #include "inc/xyz.h" and #include "driverlib/xxyyzz.h"
//
#include "inc/lm3s6965.h" 
//#include "lm3s6911.h" 

//#include "inc/hw_ssi.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
//#include "inc/hw_ethernet.h"
//#include "hw_sysctl.h"


#include "driverlib/sysctl.h"
//#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/ssi.h"
//#include "driverlib/flash.h"
#include "driverlib/uart.h"
//#include "driverlib/ethernet.h"
//#include "driverlib/watchdog.h"
//#include "driverlib/systick.h"

#include "lm3s6965-uart.h"




//////////////////////////////////////////////////////////////////////////////// UART
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////// select a baud rate
// #define BAUD_RATE 9600
// #define BAUD_RATE 38400
// #define BAUD_RATE 115200



// Enable UART0
static void init_UART0(int baud_rate, int irda){
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);  // PA0->U0RX, PA1->U0TX
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), baud_rate, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  //                                                ^^^^^^^^^
  //                                                   |
  //                                                   +-- this change impacts the other 2 uarts if used,
  //                                                       or all 3 uarts use the same baud rate
  //
  UARTDisable(UART0_BASE);
  if(irda) UARTEnableSIR(UART0_BASE,(tBoolean) false); // IrDA
  while(UARTCharsAvail(UART0_BASE)) UARTCharGet(UART0_BASE); // drain the FIFO
  UARTEnable(UART0_BASE);
  UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}


// Enable UART1
static void init_UART1(int baud_rate, int irda){
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);  // PD2->U1RX, PD3->U1TX 
  GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
  UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), baud_rate, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  //                                                ^^^^^^^^^
  UARTDisable(UART1_BASE);
  if(irda) UARTEnableSIR(UART1_BASE,(tBoolean) false); // IrDA
  while(UARTCharsAvail(UART1_BASE)) UARTCharGet(UART1_BASE); // drain the FIFO
  UARTEnable(UART1_BASE);
  UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
}

// Enable UART2
static void init_UART2(int baud_rate, int irda){
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);  // GD0->U2RX, GD1->U2TX
  GPIOPinTypeUART(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), baud_rate, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  //
  UARTDisable(UART2_BASE);
  if(irda) UARTEnableSIR(UART2_BASE, (tBoolean) false); // IrDA
  while(UARTCharsAvail(UART2_BASE)) UARTCharGet(UART2_BASE); // drain the FIFO
  UARTEnable(UART2_BASE);
  UARTIntEnable(UART2_BASE, UART_INT_RX | UART_INT_RT);
}

void uC_init_UART(int uart, int baud, int irda){
  switch(uart){
  case 0: init_UART0(baud,irda); break;
  case 1: init_UART1(baud,irda); break;
  case 2: init_UART2(baud,irda); break;
  }
}

void uart_isr_enable(int uart){
  switch(uart){
  case 0: IntEnable(INT_UART0); break;
  case 1: IntEnable(INT_UART1);	break;
  case 2: IntEnable(INT_UART2); break;
  }
}


//-------------------------------------------- serial I/O
// I can't decide if I should use the NonBlocking version CharPut
// emit a single character
static void emit0(char c){UARTCharPut(UART0_BASE, c);}
static void emit1(char c){UARTCharPut(UART1_BASE, c);}
static void emit2(char c){UARTCharPut(UART2_BASE, c);}

void uC_emit(char c, int uart){
  switch(uart){
  case 0: emit0(c); break;
  case 1: emit1(c); break;
  case 2: emit2(c); break;
  }
}

// string OUTPUT
//////////////////////////////////////////////////////////////////////////////////////
static void print0(char *c){ while(*c) emit0(*c++); }
static void print1(char *c){ while(*c) emit1(*c++); }
static void print2(char *c){ while(*c) emit2(*c++); }

void uC_print(char *s, int uart){
  switch(uart){
  case 0: print0(s); break;
  case 1: print1(s); break;
  case 2: print2(s); break;
  }
}


// transulate NL to CR (!! for HYPERTERM !!)
// THIS SHOULD BE A CONFIGURATION OPTION
//
// I have been looking for this "bug" for some time !!!
// It looks like the proper method is to use \n\r in all of my console output !!!
// void print0(char *c){
//   while(*c){
//     //    if(*c == '\n') emit0('\r'); else
//       emit0(*c);
//     c++;
//   }
// }


static int write0(char* buf, int len){ int i; for(i=0;i<len;i++) emit0(buf[i]); return i; }
static int write1(char* buf, int len){ int i; for(i=0;i<len;i++) emit1(buf[i]); return i; }
static int write2(char* buf, int len){ int i; for(i=0;i<len;i++) emit2(buf[i]); return i; }

int uC_write(int uart, char* buf, int len){
  switch(uart){
  case 0 : return write0(buf,len);
  case 1 : return write1(buf,len);
  case 2 : return write2(buf,len);
  }
  return 0;
}

// these 3 functions BLOCK !!!!!!!
//
//
// static int read0(void){ while(uart0_kbhit() == 0); return uart0_getc(); }
// static int read1(void){ while(uart1_kbhit() == 0); return uart1_getc(); }
// static int read2(void){ while(uart2_kbhit() == 0); return uart2_getc(); }
// 
// int read(int uart, char* buf, int len){
//   int x;
//   int cnt;
// 
//   cnt = 0;
//   while(cnt < len){
//     switch(uart){
//     case 0 : x = read0(); break;
//     case 1 : x = read1(); break;
//     case 2 : x = read2(); break;
//     default: x = 0;
//     }
//     
//     write(uart,(char*)&x,1);
//     buf[cnt++] = x;
//     if(x == '\n'){ write(uart,"\r",1); goto done; }
//     if(x == '\r'){ write(uart,"\n",1); goto done; }
//   }
//  done:
//   buf[cnt] = 0;
//   return cnt;
// }


// character INPUT
/////////////////////////////////////////////////////////////////////////////////
// uart queues                                                                 //
/////////////////////////////////////////////////////////////////////////////////
#define QUEUE_0_SIZE 128						       //
#define QUEUE_1_SIZE 128						       //
#define QUEUE_2_SIZE 128						       //
                                                                               //
static unsigned char uq0[QUEUE_0_SIZE];                                        //
static int uq0in = 0;                                                          //
static int uq0out = 0;                                                         //
                                                                               //
static unsigned char uq1[QUEUE_1_SIZE];                                        //
static int uq1in = 0;                                                          //
static int uq1out = 0;                                                         //
                                                                               //
static unsigned char uq2[QUEUE_2_SIZE];                                        //
static int uq2in = 0;                                                          //
static int uq2out = 0;                                                         //
/////////////////////////////////////////////////////////////////////////////////
// The UART0 interrupt handler.
/////////////////////////////////////////////////////////////////////////////////
void uart0_isr(void){                                                          //
  unsigned long status;                                                        //
  int x;                                                                       //
                                                                               //
  status = UARTIntStatus(UART0_BASE, true);                                    // Get the interrrupt status.
  UARTIntClear(UART0_BASE, status);                                            // Clear the asserted interrupts.
                                                                               //
  while(UARTCharsAvail(UART0_BASE)){                                           // Loop while there are characters in the receive FIFO.
    x = (unsigned char)UARTCharGetNonBlocking(UART0_BASE);                     // fetch char into buffer
    uq0[uq0in%QUEUE_0_SIZE] = x;                                               //
    uq0in++;                                                                   // increment counter
  }                                                                            //
}                                                                              //
/////////////////////////////////////////////////////////////////////////////////

// The UART1 interrupt handler. 
/////////////////////////////////////////////////////////////////////////////////
void uart1_isr(void){                                                          //
  unsigned long status;                                                        //
  int x;
  status = UARTIntStatus(UART1_BASE, true);                                    // Get the interrrupt status.
  UARTIntClear(UART1_BASE, status);                                            // Clear the asserted interrupts.
   while(UARTCharsAvail(UART1_BASE)){                                          // Loop while there are characters in the receive FIFO.
     x = (unsigned char)UARTCharGetNonBlocking(UART1_BASE);                    // fetch char
     uq1[uq1in%QUEUE_1_SIZE] = x;                                              // fetch char into buffer
     uq1in++;                                                                  // increment counter
  }                                                                            //
}                                                                              //
/////////////////////////////////////////////////////////////////////////////////


// The UART2 interrupt handler. 
/////////////////////////////////////////////////////////////////////////////////
void uart2_isr(void){                                                          //
  unsigned long status;                                                        //
  int x;
  status = UARTIntStatus(UART2_BASE, true);                                    // Get the interrrupt status.
  UARTIntClear(UART2_BASE, status);                                            // Clear the asserted interrupts.
   while(UARTCharsAvail(UART2_BASE)){                                          // Loop while there are characters in the receive FIFO.
     x = (unsigned char)UARTCharGetNonBlocking(UART2_BASE);                    // fetch char
     uq2[uq2in%QUEUE_2_SIZE] = x;                                              // fetch char into buffer
     uq2in++;                                                                  // increment counter
  }                                                                            //
}                                                                              //
/////////////////////////////////////////////////////////////////////////////////

static int uart0_kbhit(void){ return UARTCharsAvail(UART0_BASE); }
static int uart1_kbhit(void){ return UARTCharsAvail(UART1_BASE); }
static int uart2_kbhit(void){ return UARTCharsAvail(UART2_BASE); }

int uC_kbhit(int uart){
  switch(uart){
  case 0: return uart0_kbhit();
  case 1: return uart1_kbhit();
  case 2: return uart2_kbhit();
  }
  return 0;
}

static int uart0_getc(void){ return UARTCharGetNonBlocking(UART0_BASE); }
static int uart1_getc(void){ return UARTCharGetNonBlocking(UART1_BASE); }
static int uart2_getc(void){ return UARTCharGetNonBlocking(UART2_BASE); }

int uC_getc(int uart){
  switch(uart){
  case 0: return uart0_getc();
  case 1: return uart1_getc();
  case 2: return uart2_getc();
  }
  return 0;
}



// Currently gets0 returns on NL or CR, but does not save the NL or CR in
// tib.  We simply return. So it seems it should be the CLI that issues CR
// to HYPERTERM

// control = 0 -- echo character
//          -1 -- hide character with *
//          -2 -- show nothing

char* uC_fgets(char *s, int size, int uart){
  int x;
  int n;

  n = 0;
  while(1){
    while(uC_kbhit(uart)==0);
    x = uC_getc(uart);
    if(x == '\n'){ s[n] = 0x00; goto done; }  // return on NL    
    if(x == '\r'){ s[n] = 0x00; goto done; }  // return on CR
    if(x == '\b' || x == 127){
      if(n>0){
	uC_emit('\b',uart);
	uC_emit(' ',uart);
	uC_emit('\b',uart);
	n--;
      }
      continue;
    }
    if(n < size-1) s[n++] = x; // place char into buffer
    uC_emit(x,uart);           // echo character placed into buffer
    if(n == size-1){ s[n] = 0; goto done; }
  }
 done:
  return s;
}

static int _console_ = -1;

// !!!! I could get in trouble here if I select a "console" that has not been initialized
// 
void uC_set_console(int x){
  switch(x){
  case 0: _console_ = 0; break;
  case 1: _console_ = 1; break;
  case 2: _console_ = 2; break;
  }
}


#include <stdarg.h>




//*****************************************************************************
//
//! A simple UART based printf function supporting \%c, \%d, \%p, \%s, \%u,
//! \%x, and \%X.
//!
//! \param pcString is the format string.
//! \param ... are the optional arguments, which depend on the contents of the
//! format string.
//!
//! This function is very similar to the C library <tt>fprintf()</tt> function.
//! All of its output will be sent to the UART.  Only the following formatting
//! characters are supported:
//!
//! - \%c to print a character
//! - \%d or \%i to print a decimal value
//! - \%s to print a string
//! - \%u to print an unsigned decimal value
//! - \%x to print a hexadecimal value using lower case letters
//! - \%X to print a hexadecimal value using lower case letters (not upper case
//! letters as would typically be used)
//! - \%p to print a pointer as a hexadecimal value
//! - \%\% to print out a \% character
//!
//! For \%s, \%d, \%i, \%u, \%p, \%x, and \%X, an optional number may reside
//! between the \% and the format character, which specifies the minimum number
//! of characters to use for that value; if preceded by a 0 then the extra
//! characters will be filled with zeros instead of spaces.  For example,
//! ``\%8d'' will use eight characters to print the decimal value with spaces
//! added to reach eight; ``\%08d'' will use eight characters as well but will
//! add zeroes instead of spaces.
//!
//! The type of the arguments after \e pcString must match the requirements of
//! the format string.  For example, if an integer was passed where a string
//! was expected, an error of some kind will most likely occur.
//!
//! \return None.
//
//*****************************************************************************

static const char * const g_pcHex = "0123456789abcdef";

void uC_printf(const char *pcString, ...){
  unsigned long ulIdx, ulValue, ulPos, ulCount, ulBase, ulNeg;
  char *pcStr, pcBuf[16], cFill;
  va_list vaArgP;


  // Start the varargs processing.
  //
  va_start(vaArgP, pcString);


  // Loop while there are more characters in the string.
  //
  while(*pcString){
    // Find the first non-% character, or the end of the string.
    //
    for(ulIdx = 0; (pcString[ulIdx] != '%') && (pcString[ulIdx] != '\0');
	ulIdx++)
      {
      }

    //
    // Write this portion of the string.
    //
    uC_write(_console_,pcString, ulIdx);

    //
    // Skip the portion of the string that was written.
    //
    pcString += ulIdx;

    //
    // See if the next character is a %.
    //
    if(*pcString == '%'){
      //
      // Skip the %.
      //
      pcString++;

      //
      // Set the digit count to zero, and the fill character to space
      // (i.e. to the defaults).
      //
      ulCount = 0;
      cFill = ' ';
      
      //
      // It may be necessary to get back here to process more characters.
      // Goto's aren't pretty, but effective.  I feel extremely dirty for
      // using not one but two of the beasts.
      //
    again:

      //
      // Determine how to handle the next character.
      //
      switch(*pcString++){
	//
	// Handle the digit characters.
	//
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	{
	  //
	  // If this is a zero, and it is the first digit, then the
	  // fill character is a zero instead of a space.
	  //
	  if((pcString[-1] == '0') && (ulCount == 0))
	    {
	      cFill = '0';
	    }

	  //
	  // Update the digit count.
	  //
	  ulCount *= 10;
	  ulCount += pcString[-1] - '0';

	  //
	  // Get the next character.
	  //
	  goto again;
	}

	//
	// Handle the %c command.
	//
      case 'c':
	{
	  //
	  // Get the value from the varargs.
	  //
	  ulValue = va_arg(vaArgP, unsigned long);
	  
	  //
	  // Print out the character.
	  //
	  uC_write(_console_,(char *)&ulValue, 1);
	  
	  //
	  // This command has been handled.
	  //
	  break;
	}

	      //
	      // Handle the %d and %i commands.
	      //
	    case 'd':
	    case 'i':
	      {
		//
		// Get the value from the varargs.
		//
		ulValue = va_arg(vaArgP, unsigned long);

		//
		// Reset the buffer position.
		//
		ulPos = 0;

		//
		// If the value is negative, make it positive and indicate
		// that a minus sign is needed.
		//
		if((long)ulValue < 0)
		  {
		    //
		    // Make the value positive.
		    //
		    ulValue = -(long)ulValue;

		    //
		    // Indicate that the value is negative.
		    //
		    ulNeg = 1;
		  }
		else
		  {
		    //
		    // Indicate that the value is positive so that a minus
		    // sign isn't inserted.
		    //
		    ulNeg = 0;
		  }

		//
		// Set the base to 10.
		//
		ulBase = 10;

		//
		// Convert the value to ASCII.
		//
		goto convert;
	      }

	      //
	      // Handle the %s command.
	      //
	    case 's':
	      {
		//
		// Get the string pointer from the varargs.
		//
		pcStr = va_arg(vaArgP, char *);

		//
		// Determine the length of the string.
		//
		for(ulIdx = 0; pcStr[ulIdx] != '\0'; ulIdx++)
		  {
		  }

		//
		// Write the string.
		//
		uC_write(_console_,pcStr, ulIdx);

		//
		// Write any required padding spaces
		//
		if(ulCount > ulIdx)
		  {
		    ulCount -= ulIdx;
		    while(ulCount--)
		      {
			uC_write(_console_," ", 1);
		      }
		  }
		//
		// This command has been handled.
		//
		break;
	      }

	      //
	      // Handle the %u command.
	      //
	    case 'u':
	      {
		//
		// Get the value from the varargs.
		//
		ulValue = va_arg(vaArgP, unsigned long);

		//
		// Reset the buffer position.
		//
		ulPos = 0;

		//
		// Set the base to 10.
		//
		ulBase = 10;

		//
		// Indicate that the value is positive so that a minus sign
		// isn't inserted.
		//
		ulNeg = 0;

		//
		// Convert the value to ASCII.
		//
		goto convert;
	      }

	      //
	      // Handle the %x and %X commands.  Note that they are treated
	      // identically; i.e. %X will use lower case letters for a-f
	      // instead of the upper case letters is should use.  We also
	      // alias %p to %x.
	      //
	    case 'x':
	    case 'X':
	    case 'p':
	      {
		//
		// Get the value from the varargs.
		//
		ulValue = va_arg(vaArgP, unsigned long);

		//
		// Reset the buffer position.
		//
		ulPos = 0;

		//
		// Set the base to 16.
		//
		ulBase = 16;

		//
		// Indicate that the value is positive so that a minus sign
		// isn't inserted.
		//
		ulNeg = 0;

		//
		// Determine the number of digits in the string version of
		// the value.
		//
	      convert:
		for(ulIdx = 1;
		    (((ulIdx * ulBase) <= ulValue) &&
		     (((ulIdx * ulBase) / ulBase) == ulIdx));
		    ulIdx *= ulBase, ulCount--)
		  {
		  }

		//
		// If the value is negative, reduce the count of padding
		// characters needed.
		//
		if(ulNeg)
		  {
		    ulCount--;
		  }

		//
		// If the value is negative and the value is padded with
		// zeros, then place the minus sign before the padding.
		//
		if(ulNeg && (cFill == '0'))
		  {
		    //
		    // Place the minus sign in the output buffer.
		    //
		    pcBuf[ulPos++] = '-';

		    //
		    // The minus sign has been placed, so turn off the
		    // negative flag.
		    //
		    ulNeg = 0;
		  }

		//
		// Provide additional padding at the beginning of the
		// string conversion if needed.
		//
		if((ulCount > 1) && (ulCount < 16))
		  {
		    for(ulCount--; ulCount; ulCount--)
		      {
			pcBuf[ulPos++] = cFill;
		      }
		  }

		//
		// If the value is negative, then place the minus sign
		// before the number.
		//
		if(ulNeg)
		  {
		    //
		    // Place the minus sign in the output buffer.
		    //
		    pcBuf[ulPos++] = '-';
		  }

		//
		// Convert the value into a string.
		//
		for(; ulIdx; ulIdx /= ulBase)
		  {
		    pcBuf[ulPos++] = g_pcHex[(ulValue / ulIdx) % ulBase];
		  }

		//
		// Write the string.
		//
		uC_write(_console_,pcBuf, ulPos);

		//
		// This command has been handled.
		//
		break;
	      }

	      //
	      // Handle the %% command.
	      //
	    case '%':
	      {
		//
		// Simply write a single %.
		//
		uC_write(_console_,pcString - 1, 1);

		//
		// This command has been handled.
		//
		break;
	      }

	      //
	      // Handle all other commands.
	      //
	    default:
	      {
		//
		// Indicate an error.
		//
		uC_write(_console_,"ERROR", 5);

		//
		// This command has been handled.
		//
		break;
	      }
            }
        }
    }

  //
  // End the varargs processing.
  //
  va_end(vaArgP);
}
