// syslog.c
//
// notes: This syslog() uses uC_printf()
//
//
//
#include <stdarg.h>
#include "syslog.h"
//
//
//
//
static int _syslog = 0;
//
//
//
static char* pre_fix(int x){
  switch(x){
  case LOG_EMERG   : return "<x> ";
  case LOG_ALERT   : return "<a> ";
  case LOG_CRIT    : return "<c> ";
  case LOG_ERR     : return "<e> ";
  case LOG_WARNING : return "<w> ";
  case LOG_NOTICE  : return "<n> ";
  case LOG_INFO    : return "<i> ";
  case LOG_DEBUG   : return "<d> ";
  case LOG_PRINT   : return "<X> ";
  }
  return "<?>";
}
//
//
//
//#warning "syslog is now broken"

void syslog(int prio, char *template, ...){
  static char msg[128]; 
  va_list ap; 

  if(prio == LOG_PRINT) goto print; // ALWAYS print
  if(_syslog == 0) return; // NEVER print
  if(prio & _syslog) goto print; // print requested level
  return;
   
 print:
  va_start(ap, template); 
  uC_printf("\n\r%s",pre_fix(prio));
  uC_printf(template, ap); 
  va_end(ap); 

}
//
//
//
void syslog_level_set(int x){
  _syslog = x;
  syslog(LOG_PRINT,"syslog=0x%04X",x); // issue message anytime syslog level changes !!
}
//
//
//
int syslog_level_get(void){
  return _syslog;
}
//
//
//
static char* sys_help_string = "\r\n\
<x> LOG_EMERG   0x0001\r\n\
<a> LOG_ALERT   0x0002\r\n\
<c> LOG_CRIT    0x0004\r\n\
<e> LOG_ERR     0x0010\r\n\
<w> LOG_WARNING 0x0020\r\n\
<n> LOG_NOTICE  0x0040\r\n\
<i> LOG_INFO    0x0080\r\n\
<d> LOG_DEBUG   0x0100\r\n\
<X> LOG_PRINT   0xFFFF\r\n\
    LOG_ALL     0x01FF\r\n\
    LOG_OFF     0x0000\r\n\
";

//char* syslog_help(void){ return sys_help_string; }

char* syslog_help(void){
  uC_printf("\r\nsyslog=%X %s",_syslog,sys_help_string); }
