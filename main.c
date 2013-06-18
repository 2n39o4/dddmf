// main.c
//
//
// view init-uC.c to see what uart the console is on and the BAUD rate !!!
// run this code on the lm3s6965 SDK board. There is an LED on 
//
// BUGS: +) I notice that the console misses characters. Investigate !!!
//       +)
//
#include "syslog.h"
//#include <stdlib.h>
//
//
//
extern void init_uC(void);
extern void uC_init_SYSTICK(int);
extern void init_task_list(void);
extern void print(char*);
extern void run_tasks(void);
extern void PRINTF(char*,...);
extern void dddmf_main(void);
//
//
void update_tasks(void){}
//
//
//
//
static char* greet[] = {
  "\n\n\n\n\r + =================== BANNER ===================",
  "\n\r + lm3s6965 dev kit ",
  "\n\r + version 1.0.0_rc1 COMPILED ", __DATE__," / ", __TIME__,
  (char*)0
};
//
//
//
static void greeting(void){
  int i;
  for(i=0;greet[i];i++){
    uC_print(greet[i]);
  }
}
//
//
//
extern unsigned int _data;
extern unsigned int _edata;
extern unsigned int _ebss;

extern int get_stack_top(void);
extern int get_stack_bottom(void);

//
//
//
static void mem_info(void){
  int x;

  syslog(LOG_PRINT,"TOS=0x%X", x=get_stack_top());
  syslog(LOG_PRINT,"BOS=0x%X", x=get_stack_bottom());
  syslog(LOG_PRINT,"_data=%X",&_data);
  syslog(LOG_PRINT,"_edata=%X",&_edata);
  syslog(LOG_PRINT,"_ebss=%X",&_ebss);
  // the above does not give good results.
  // _data, _edata, _ebss report the same value (0x200004d8)
  //
  // !!! it looks like the trouble is in syslog() !!!!!


  //  x = malloc(2048); free(x);
  //  x = malloc(1048); free(x);

  //  syslog(LOG_PRINT,"mem_info: x=%X",x);

  //  return x;
}
//
//
//
void main(void){

  init_uC();
  uC_set_console(0);
  greeting();
  syslog_level_set(LOG_ALL);
  mem_info();

  dddmf_main();
  //  syslog_print_set(print);  // this tells syslog what function to 
                            // call to "print" info to the console

  //  init_task_list();
  //  uC_init_SYSTICK(1000);
  //  while(1){
  //    run_tasks();
  //  }
}




#if 0
#include <stdio.h>
#include <string.h>
//
//#include "syslog.h"
//
//
//
//extern void init_uC(int uart, int baud);
extern void init_uC(void);
extern void init_task_list(void);
extern void uC_init_SYSTICK(int);
extern void RIT128x96x4Init(int);
extern void RIT128x96x4StringDraw(char*,int,int,int);
extern void run_tasks(void);
//
//
//
extern unsigned int _etext;
extern unsigned int _edata;
//
void memory_map(void){
  syslog(LOG_INFO,"_etext=0x%X", &_etext);
  syslog(LOG_INFO,"_edata=0x%X", &_edata);
}
//
//
//
//volatile int zamboni[32];
static int fred = 0xC0FFEE;
//static int bob = 0x12345678;
volatile int zamboni = 0xaaaaaaaa;

//
//
//
int main(void){
 
  //  init_uC(0,115200);
  init_uC();

    //  init_PORTF();

    // init_ADC();

  RIT128x96x4Init(1000000);
  RIT128x96x4StringDraw("Hello World!", 30, 24, 15);

  syslog_level_set(LOG_ALL);

  
  syslog(LOG_PRINT,"fred=%X (%X)",fred,&fred);

  memory_map();

  init_task_list();

  uC_init_SYSTICK(1000);

  while(1){
    run_tasks();
  }

}

#endif
