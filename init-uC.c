// init-uC.c
//
//
// notes: Currently only 1 uart gets initialized.  What if I want to use all 3 uarts ???
//        NEW FUNCTION => init_UARTs(x,y,z)
//
// notes: use set_console(x) to declare/define a console uart. Once set_console has been called
//        use uC_kbhit, uC_getc, uC_emit, uC_print, uC_key
//
#include "liblm3s6965.h"
//#include "syslog.h"
//
//
//
//extern void set_console(int);
extern void uC_printf(char*,...);
#define PRINTF uC_printf

//
//
#define CONSOLE   0
#define BAUD_RATE 19200

#define U0_BAUD   115200
#define U1_BAUD   19200
#define U2_BAUD   0

//
//
// Specify uart (0,1,2) and baudrate (115200)
//
static void init_UART(int uart, int baud){
  //  int x;

  // 1) Need to init the system clock first
  // this ONLY needs to be done ONCE !!!  uC_init_CLOCK(50); // 50Mhz

  // 2) Next I want a serial port so I can start issuing status to the console
  uC_init_UART(uart,baud,0);

  // set_console(uart); // !! this tells PRINTF which uart to use !!!
  // I have to move this outside of init_UART() OTHERWISE it will be called multiple
  // times if I init multiple uarts !!!!!


  // 3) Announce thyself on the console
  //  PRINTF("\r\n=========================================================== init_uC");

  // 4) show the device id on the console
  //  x = uC_id(0); PRINTF("\r\n - DID[0]=0x%X ",x);
  //  x = uC_id(1); PRINTF("DID[1]=0x%X",x);

  // 5) show the uC clock speed 
  //  x = uC_CLOCK();
  // PRINTF("\r\n - sys_clock=%d", x);

  // 6) anounce baud rate. The baud rate MAY change after the system config structure is loaded
  //  PRINTF("\r\n - baudrate=%d", baud);

  // 7) initialize the FLASH controler
  //  uC_init_FLASH();
  //  PRINTF("\r\n - init_FLASH()");

  // 8) enable UART interrupts for console INPUT !!
  //  uart_isr_enable(uart);
}

static void init_UARTs(int u0baud, int u1baud, int u2baud){
  if(u0baud) init_UART(0,u0baud);
  if(u1baud) init_UART(1,u1baud);
  if(u0baud) init_UART(2,u2baud);
}

//       +---+---+---+---+---+---+---+---+
// portF | X | X | X | X | 3 | 2 | 1 | 0 |
//       +---+---+---+---+---+---+---+---+
//                         |   |   |   |
//                         |   |   |   |
//                         |   |   |   |
//                         |   |   |   |
//                         |   |   |   +--- LED2 heart beat
//                         |   |   +------- IDX1
//                         |   +----------- LED1 ethernet
//                         +--------------- LED0 ethernet
//
#define PORTF_MASK 0x0F
//
void init_uC(void){
  int x;

  uC_init_CLOCK(50);

  //init_UART(CONSOLE,BAUD_RATE);
  //init_UARTs(115200,9600,9600);

  init_UARTs(U0_BAUD,U1_BAUD,U2_BAUD);

  // after all the uarts are initialized, select one for the console
  uC_set_console(CONSOLE); // !! this tells PRINTF which uart to use !!!

  uC_printf("\r\n=========================================================== init_uC");

  x = uC_CLOCK();
  uC_printf("\r\n + sys_clock=%d", x);
  uC_printf("\r\n + uart0_baudrate=%d", U0_BAUD);
  uC_printf("\r\n + uart1_baudrate=%d", U1_BAUD);
  uC_printf("\r\n + uart2_baudrate=%d", U2_BAUD);



  uC_gpio_output('f',PORTF_MASK,2);
  uC_gpio_output('d',0x02,2);
  //  uC_gpio_output('f',0x0F,2);
  //              |  ---- |
  //              |    |  +--- drive (mA)
  //              |    +------ pin
  //              +----------- port
  //  init_PORTF();
  //  init_ADC();

  //  uC_gpio_output('b',0x02,2);
  //  uC_gpio_output('b',0x04,2);

  uC_init_SYSTICK(1000);

}
