// lm3s6965-gpio.c
//
//
//
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
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                            +------- port                                       //
//                           /                                                    //
//                          /           +---- pin                                 //
//                         /           /                                          //
//                        /           /                                           //
//                 --------------- ----------                                     //
#define        _CS0_ GPIO_PORTA_BASE,GPIO_PIN_6  // PORT A                        //
#define        _CS1_ GPIO_PORTA_BASE,GPIO_PIN_7                                   //
#define        _SCL_ GPIO_PORTA_BASE,GPIO_PIN_2     // SERIAL CLOCK               //
#define         _SO_ GPIO_PORTA_BASE,GPIO_PIN_4     // SERIAL OUT                 //
#define         _SI_ GPIO_PORTA_BASE,GPIO_PIN_5     // SERIAL IN                  //
#define         _SF_ GPIO_PORTA_BASE,GPIO_PIN_3     // SPI FRAME                  //
////////////////////////////////////////////////////////////////////////////////////



                                                                                  //
#define        _MDC_ GPIO_PORTC_BASE,GPIO_PIN_4  // PORT C                        //
#define       _MDIO_ GPIO_PORTC_BASE,GPIO_PIN_5                                   //
#define   _SW_RESET_ GPIO_PORTC_BASE,GPIO_PIN_6                                   //
#define     _INT_SW_ GPIO_PORTC_BASE,GPIO_PIN_7                                   //
/////////////////////////////////////////////////// PORT E /////////////////////////
#define    _RED_LED_ GPIO_PORTE_BASE,GPIO_PIN_1                                   //
#define   _BLUE_LED_ GPIO_PORTE_BASE,GPIO_PIN_0                                   //
#define _YELLOW_LED_ GPIO_PORTE_BASE,GPIO_PIN_2                                   //
#define    _XTAL_EN_ GPIO_PORTE_BASE,GPIO_PIN_3                                   //
////////////////////////////////////////////////// PORT D //////////////////////////
#define  _ENABLE_33_ GPIO_PORTD_BASE,GPIO_PIN_0           // U720   1.2  1.5      //
#define  _ENABLE_25_ GPIO_PORTD_BASE,GPIO_PIN_4           // U710   2.5  2.5      //
#define  _ENABLE_12_ GPIO_PORTD_BASE,GPIO_PIN_6           // U700   3.3  1.2      //
#define   _PGOOD_33_ GPIO_PORTD_BASE,GPIO_PIN_1                                   //
#define   _PGOOD_25_ GPIO_PORTD_BASE,GPIO_PIN_5                                   //
#define   _PGOOD_12_ GPIO_PORTD_BASE,GPIO_PIN_7                                   //

#define    _EN_U720_ GPIO_PORTD_BASE,GPIO_PIN_0           // U720   1.2  1.5      //
#define    _EN_U710_ GPIO_PORTD_BASE,GPIO_PIN_4           // U710   2.5  2.5      //
#define    _EN_U700_ GPIO_PORTD_BASE,GPIO_PIN_6           // U700   3.3  1.2      //
#define _PGOOD_U700_ GPIO_PORTD_BASE,GPIO_PIN_1                                   //
#define _PGOOD_U710_ GPIO_PORTD_BASE,GPIO_PIN_5                                   //
#define _PGOOD_U720_ GPIO_PORTD_BASE,GPIO_PIN_7                                   //

#define       _PD_0_ GPIO_PORTD_BASE,GPIO_PIN_0           // U720   1.2  1.5      //
#define       _PD_4_ GPIO_PORTD_BASE,GPIO_PIN_4           // U710   2.5  2.5      //
#define       _PD_6_ GPIO_PORTD_BASE,GPIO_PIN_6           // U700   3.3  1.2      //

////////////////////////////////////////////////////////////////////////////////////

// macros for PIN I/O
////////////////////////////////////////////////////////////////////////////////////////////////////
#define OUTPUT_PIN GPIOPinTypeGPIOOutput                                                          //
#define INPUT_PIN GPIOPinTypeGPIOInput                                                            //
                                                                                                  //
                                                                                                  //
#define PAD_CONFIG_2MA(x)  GPIOPadConfigSet(x,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU)           //
#define PIN_ON(x)          GPIOPinWrite(x,0xFF)                                                   //
#define PIN_OFF(x)         GPIOPinWrite(x,0x00)                                                   //


#define READ_PIN(x) GPIOPinRead(x)


#define LED1_PERIPH SYSCTL_PERIPH_GPIOF
#define LED1_PORT GPIO_PORTF_BASE
#define LED1_BIT GPIO_PIN_0

#define _PF_0_ GPIO_PORTF_BASE,GPIO_PIN_0

void init_PORTF(void){
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  PAD_CONFIG_2MA(_PF_0_);
  PIN_OFF(_PF_0_);
  OUTPUT_PIN(_PF_0_);
}

//void toggle_port_f_bit_0(void){
//  static int x;
//  if(x & 1) GPIOPinWrite(GPIO_PORTF_BASE, 0x0F, 0xFF);
//  else      GPIOPinWrite(GPIO_PORTF_BASE, 0x0F, 0);
//  x++;
//}


void toggle_port_f_bit_0(void){
  static int x;
  if(x & 1) GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
  else      GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
  x++;
}


void toggle_port_b_bit_0(void){
  static int x;
  if(x & 1) GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
  else      GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
  x++;
}

void uC_PF_on(int bits){
  //  GPIOPinWrite(GPIO_PORTF_BASE, bits, 0xFF);
  GPIOPinWrite(GPIO_PORTF_BASE, bits, bits);
}

void uC_PF_off(int bits){
  GPIOPinWrite(GPIO_PORTF_BASE, bits, 0x00);
}

void uC_PD_on(int bits){
  GPIOPinWrite(GPIO_PORTD_BASE, bits, bits);
}

void uC_PD_off(int bits){
  GPIOPinWrite(GPIO_PORTD_BASE, bits, 0x00);
}


#define OUTPUT_PINS GPIOPinTypeGPIOOutput                                                          //

void uC_gpio_output(int port, int pins,int drive){
  int enable;
  int base;
  int ma;

  switch(drive){
  case 2: ma = GPIO_STRENGTH_2MA; break;
  case 4: ma = GPIO_STRENGTH_4MA; break;
  case 8: ma = GPIO_STRENGTH_8MA; break;
  default: ma = GPIO_STRENGTH_8MA;
  }

  switch(port){
  case 'a': enable = SYSCTL_PERIPH_GPIOA; base = GPIO_PORTA_BASE; break;
  case 'b': enable = SYSCTL_PERIPH_GPIOB; base = GPIO_PORTB_BASE; break;
  case 'c': enable = SYSCTL_PERIPH_GPIOC; base = GPIO_PORTC_BASE; break;
  case 'd': enable = SYSCTL_PERIPH_GPIOD; base = GPIO_PORTD_BASE; break;
  case 'e': enable = SYSCTL_PERIPH_GPIOE; base = GPIO_PORTE_BASE; break;
  case 'f': enable = SYSCTL_PERIPH_GPIOF; base = GPIO_PORTF_BASE; break;
    //  default: print("INVALID PORT"); return;
  }
  SysCtlPeripheralEnable(enable);
  GPIOPinWrite(base,pins,0x00);
  GPIOPinTypeGPIOOutput(base,pins);
  GPIOPadConfigSet(base,pins,ma, GPIO_PIN_TYPE_STD);
  
  //  OUTPUT_PINS(base,pins);
}

void uC_gpio_toggle(int port, int pin){
  int base;
  int x;
  switch(port){
  case 'a': base = GPIO_PORTA_BASE; break;
  case 'b': base = GPIO_PORTB_BASE; break;
  case 'c': base = GPIO_PORTC_BASE; break;
  case 'd': base = GPIO_PORTD_BASE; break;
  case 'e': base = GPIO_PORTE_BASE; break;
  case 'f': base = GPIO_PORTF_BASE; break;
    //  default: print("INVALID PORT"); return;
  }
  x = GPIOPinRead(base,pin);
  if(x) GPIOPinWrite(base,pin,0x00);
  else  GPIOPinWrite(base,pin,0xff);
}


